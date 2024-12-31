#include "plugin.hpp"
#include <fmt/core.h>
#include <chrono>

class PluginImpl : public Plugin {
public:
    std::vector<Endpoint> getEndpoints() const override {
        return {
            {"/time", "GET", "Get current time"},
            {"/hello", "GET", "Get greeting"},
            {"/echo", "POST", "Echo back the request body"},
            {"/new", "GET", "New hot-reloaded endpoint!"}
        };
    }
    
    std::string handleRequest(std::string_view path, 
                            std::string_view method,
                            std::string_view body) override {
        if (path == "/time" && method == "GET") {
            return fmt::format("🕒 Current time: {}", 
                std::chrono::system_clock::now().time_since_epoch().count());
        }
        else if (path == "/hello" && method == "GET") {
            return "👋 Hello from hot-reloaded plugin!";
        }
        else if (path == "/echo" && method == "POST") {
            return fmt::format("📢 Echo: {}", body);
        }
        else if (path == "/new" && method == "GET") {
            return "🆕 This endpoint was added via hot reload!";
        }
        return "404 - Endpoint not found";
    }
};

extern "C" Plugin* createPlugin() {
    return new PluginImpl();
} 