// Include necessary headers for networking, filesystem, and utilities
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <fmt/core.h>
#include "hot_reload/interfaces.hpp"
#include <filesystem>
#include <chrono>
#include <thread>
#include <memory>


// Platform-specific dynamic library loading macros and types
#ifdef _WIN32
    // Windows dynamic library functions
    #include <windows.h>
    #define LOAD_LIBRARY(path) LoadLibrary(path)    // Load DLL
    #define GET_PROC_ADDRESS GetProcAddress         // Get function from DLL
    #define CLOSE_LIBRARY FreeLibrary               // Unload DLL
    typedef HMODULE LibraryHandle;                  // Windows DLL handle type
#else
    // Unix/Linux/MacOS dynamic library functions
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW)  // Load shared library
    #define GET_PROC_ADDRESS dlsym                      // Get function from shared library
    #define CLOSE_LIBRARY dlclose                       // Unload shared library
    typedef void* LibraryHandle;                        // Unix shared library handle type
#endif

// Namespace aliases for cleaner code
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// Handles loading, unloading and managing plugin libraries
class PluginLoader {
public:
    // Initialize with no loaded plugin
    PluginLoader() : handle_(nullptr), plugin_(nullptr) {}
    
    // Load a plugin from the specified path
    bool loadPlugin(const std::string& path) {
        fmt::print("Loading plugin: {}\n", path);
        
        // Clean up existing plugin if one is loaded
        if (handle_) {
            delete plugin_;
            CLOSE_LIBRARY(handle_);
        }

        // Load the new plugin library
        handle_ = LOAD_LIBRARY(path.c_str());
        if (!handle_) {
            fmt::print("Failed to load library: {}\n", 
                #ifdef _WIN32
                    GetLastError()
                #else
                    dlerror()
                #endif
            );
            return false;
        }

        // Get the plugin creation function
        auto createFunc = (Plugin*(*)())GET_PROC_ADDRESS(handle_, "createPlugin");
        if (!createFunc) {
            CLOSE_LIBRARY(handle_);
            handle_ = nullptr;
            return false;
        }

        // Create plugin instance
        plugin_ = createFunc();
        fmt::print("Plugin loaded successfully\n");
        return true;
    }

    // Get current plugin instance
    Plugin* getPlugin() { return plugin_; }

private:
    LibraryHandle handle_;   // Handle to loaded library
    Plugin* plugin_;         // Current plugin instance
};

// Main HTTP server class
class HttpServer {
    // Forward declare Session class
    class Session;
    
public:
    // Initialize server with io_context and port
    HttpServer(net::io_context& ioc, unsigned short port) 
        : acceptor_(ioc, {net::ip::make_address("127.0.0.1"), port}),
          loader_() {
        
        // Determine plugin path based on platform
        std::filesystem::path pluginPath;
        #ifdef _WIN32
            pluginPath = "plugin.dll";
        #elif __APPLE__
            pluginPath = "libmanager.dylib";
        #else
            pluginPath = "libplugin.so";
        #endif

        // Load initial plugin
        if (!loader_.loadPlugin(pluginPath.string())) {
            throw std::runtime_error(fmt::format("Failed to load initial plugin from {}", pluginPath.string()));
        }

        // Start watching for plugin changes
        startPluginWatcher(pluginPath);
    }

    // Start accepting connections
    void run() {
        accept();
        fmt::print("Server running on http://localhost:63090\n");
    }

private:
    // Accept incoming connections
    void accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(*this, std::move(socket))->run();
                }
                accept();
            });
    }

    // Start background thread to watch for plugin changes
    void startPluginWatcher(const std::filesystem::path& pluginPath) {
        watcher_ = std::thread([this, pluginPath]() {
            auto lastWrite = std::filesystem::last_write_time(pluginPath);
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                try {
                    // Check if plugin file has been modified
                    auto currentWrite = std::filesystem::last_write_time(pluginPath);
                    if (currentWrite != lastWrite) {
                        fmt::print("Plugin changed, reloading...\n");
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        if (loader_.loadPlugin(pluginPath.string())) {
                            lastWrite = currentWrite;
                        }
                    }
                } catch (const std::exception& e) {
                    fmt::print("Watcher error: {}\n", e.what());
                }
            }
        });
        watcher_.detach();  // Let thread run independently
    }

    // Handles individual HTTP connections
    class Session : public std::enable_shared_from_this<Session> {
    public:
        // Initialize session with server reference and socket
        Session(HttpServer& server, tcp::socket socket)
            : server_(server), socket_(std::move(socket)) {}

        // Start reading from socket
        void run() {
            do_read();
        }

    private:
        // Asynchronously read HTTP request
        void do_read() {
            req_ = {};
            beast::http::async_read(socket_, buffer_, req_,
                [self = shared_from_this()](beast::error_code ec, std::size_t) {
                    if (!ec) {
                        self->handle_request();
                    }
                });
        }

        // Process HTTP request and route to plugin
        void handle_request() {
            http::response<http::string_body> res{http::status::ok, req_.version()};
            res.set(http::field::server, "Beast");
            res.set(http::field::content_type, "text/plain");
            
            if (auto plugin = server_.loader_.getPlugin()) {
                // Convert boost::beast::string_view to std::string_view
                std::string_view path(req_.target().data(), req_.target().size());
                std::string_view method(req_.method_string().data(), req_.method_string().size());
                std::string_view body(req_.body().data(), req_.body().size());
                
                // Let plugin handle the request
                res.body() = plugin->handleRequest(path, method, body);
                
                // Set 404 status if endpoint not found
                if (res.body() == "404 - Endpoint not found") {
                    res.result(http::status::not_found);
                }
            } else {
                res.body() = "Plugin not loaded";
                res.result(http::status::service_unavailable);
            }

            res.prepare_payload();
            do_write(std::move(res));
        }

        // Asynchronously write HTTP response
        void do_write(http::response<http::string_body>&& res) {
            auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
            beast::http::async_write(socket_, *sp,
                [self = shared_from_this(), sp](beast::error_code ec, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
        }

        HttpServer& server_;                    // Reference to parent server
        tcp::socket socket_;                    // Connection socket
        beast::flat_buffer buffer_;            // Buffer for reading
        http::request<http::string_body> req_; // HTTP request being processed
    };

    tcp::acceptor acceptor_;     // Accepts incoming connections
    PluginLoader loader_;        // Manages plugin loading/unloading
    std::thread watcher_;        // Thread for watching plugin changes
};

// Entry point
int main() {
    try {
        net::io_context ioc{1};              // IO context with 1 thread
        HttpServer server{ioc, 63090};       // Create server on port 63090
        server.run();                        // Start accepting connections
        ioc.run();                          // Run IO event loop
    } catch (const std::exception& e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }
    return 0;
}