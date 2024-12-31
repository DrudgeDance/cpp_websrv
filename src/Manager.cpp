#include "hot_reload/interfaces.hpp"
#include <filesystem>
#include <dlfcn.h>
#include <fmt/core.h>

class ApplicationManager : public Plugin {
public:
    ApplicationManager() {
        loadControllers();
    }

    std::vector<std::shared_ptr<IController>> getControllers() override {
        return controllers_;
    }

    std::string handleRequest(std::string_view path, 
                            std::string_view method,
                            std::string_view body) override {
        fmt::print("Handling request: {} {}\n", method, path);
        
        for (const auto& controller : controllers_) {
            auto router = controller->getRouter();
            if (!router) {
                fmt::print("Warning: Controller returned null router\n");
                continue;
            }

            auto endpoint = router->getEndpoint(std::string(path));
            if (endpoint) {
                auto info = endpoint->getRouteInfo();
                if (info.method == method) {
                    fmt::print("Found endpoint: {} {}\n", info.method, info.path);
                    return endpoint->handle(body);
                }
            }
        }
        return "404 - Endpoint not found";
    }

private:
    void loadControllers() {
        fmt::print("Loading controllers...\n");
        
        // Get the binary directory path
        std::filesystem::path controllerDir = std::filesystem::current_path() / "controllers";
        
        fmt::print("Looking for controllers in: {}\n", controllerDir.string());

        if (!std::filesystem::exists(controllerDir)) {
            fmt::print("Creating controller directory: {}\n", controllerDir.string());
            std::filesystem::create_directories(controllerDir);
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(controllerDir)) {
            if (entry.path().extension() == ".so" || 
                entry.path().extension() == ".dylib" ||
                entry.path().extension() == ".dll") {
                
                loadController(entry.path());
            }
        }
        
        fmt::print("Loaded {} controllers\n", controllers_.size());
    }

    void loadController(const std::filesystem::path& path) {
        fmt::print("Loading controller: {}\n", path.string());
        
        void* handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle) {
            fmt::print("Failed to load controller: {}\n", dlerror());
            return;
        }

        auto createFunc = (IController*(*)())dlsym(handle, "createController");
        if (!createFunc) {
            fmt::print("Failed to get createController function: {}\n", dlerror());
            dlclose(handle);
            return;
        }

        try {
            auto controller = std::shared_ptr<IController>(createFunc());
            controllers_.push_back(controller);
            fmt::print("Successfully loaded controller from {}\n", path.string());
        } catch (const std::exception& e) {
            fmt::print("Error creating controller: {}\n", e.what());
            dlclose(handle);
        }
    }

    std::vector<std::shared_ptr<IController>> controllers_;
};

extern "C" EXPORT Plugin* createPlugin() {
    return new ApplicationManager();
} 