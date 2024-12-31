#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <fmt/core.h>
#include "plugin.hpp"
#include <filesystem>
#include <chrono>
#include <thread>
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(path) LoadLibrary(path)
    #define GET_PROC_ADDRESS GetProcAddress
    #define CLOSE_LIBRARY FreeLibrary
    typedef HMODULE LibraryHandle;
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW)
    #define GET_PROC_ADDRESS dlsym
    #define CLOSE_LIBRARY dlclose
    typedef void* LibraryHandle;
#endif

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class PluginLoader {
public:
    PluginLoader() : handle_(nullptr), plugin_(nullptr) {}
    
    bool loadPlugin(const std::string& path) {
        fmt::print("Loading plugin: {}\n", path);
        
        if (handle_) {
            delete plugin_;
            CLOSE_LIBRARY(handle_);
        }

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

        auto createFunc = (Plugin*(*)())GET_PROC_ADDRESS(handle_, "createPlugin");
        if (!createFunc) {
            CLOSE_LIBRARY(handle_);
            handle_ = nullptr;
            return false;
        }

        plugin_ = createFunc();
        fmt::print("Plugin loaded successfully\n");
        return true;
    }

    Plugin* getPlugin() { return plugin_; }

private:
    LibraryHandle handle_;
    Plugin* plugin_;
};

class HttpServer {
    class Session;
    
public:
    HttpServer(net::io_context& ioc, unsigned short port) 
        : acceptor_(ioc, {net::ip::make_address("127.0.0.1"), port}),
          loader_() {
        
        // Load initial plugin
        std::filesystem::path pluginPath;
        #ifdef _WIN32
            pluginPath = std::filesystem::current_path() / "bin" / "plugin.dll";
        #elif __APPLE__
            pluginPath = std::filesystem::current_path() / "bin" / "libplugin.dylib";
        #else
            pluginPath = std::filesystem::current_path() / "bin" / "libplugin.so";
        #endif

        if (!loader_.loadPlugin(pluginPath.string())) {
            throw std::runtime_error("Failed to load initial plugin");
        }

        // Start plugin watcher
        startPluginWatcher(pluginPath);
    }

    void run() {
        accept();
        fmt::print("Server running on http://localhost:63090\n");
    }

private:
    void accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(*this, std::move(socket))->run();
                }
                accept();
            });
    }

    void startPluginWatcher(const std::filesystem::path& pluginPath) {
        watcher_ = std::thread([this, pluginPath]() {
            auto lastWrite = std::filesystem::last_write_time(pluginPath);
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                try {
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
        watcher_.detach();
    }

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(HttpServer& server, tcp::socket socket)
            : server_(server), socket_(std::move(socket)) {}

        void run() {
            do_read();
        }

    private:
        void do_read() {
            req_ = {};
            beast::http::async_read(socket_, buffer_, req_,
                [self = shared_from_this()](beast::error_code ec, std::size_t) {
                    if (!ec) {
                        self->handle_request();
                    }
                });
        }

        void handle_request() {
            http::response<http::string_body> res{http::status::ok, req_.version()};
            res.set(http::field::server, "Beast");
            res.set(http::field::content_type, "text/plain");
            
            if (auto plugin = server_.loader_.getPlugin()) {
                // Get the path and method from the request
                std::string_view path(req_.target());
                std::string_view method(req_.method_string());
                
                // For POST requests, get the body
                std::string_view body(req_.body());
                
                // Handle the request
                res.body() = plugin->handleRequest(path, method, body);
                
                // Set appropriate status code
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

        void do_write(http::response<http::string_body>&& res) {
            auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
            beast::http::async_write(socket_, *sp,
                [self = shared_from_this(), sp](beast::error_code ec, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
        }

        HttpServer& server_;
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
    };

    tcp::acceptor acceptor_;
    PluginLoader loader_;
    std::thread watcher_;
};

int main() {
    try {
        net::io_context ioc{1};
        HttpServer server{ioc, 63090};
        server.run();
        ioc.run();
    } catch (const std::exception& e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }
    return 0;
} 