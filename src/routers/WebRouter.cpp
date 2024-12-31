#include "hot_reload/interfaces.hpp"
#include <filesystem>
#include <dlfcn.h>
#include <fmt/format.h>
#include <set>
#include <string>

class WebRouter : public IRouter {
public:
    WebRouter() {
        loadEndpoints();
    }

    std::vector<RouteInfo> getRoutes() const override {
        std::vector<RouteInfo> routes;
        for (const auto& [_, endpoint] : endpoints_) {
            routes.push_back(endpoint->getRouteInfo());
        }
        return routes;
    }

    std::shared_ptr<IEndpoint> getEndpoint(const std::string& path) override {
        checkForUpdates();
        auto it = endpoints_.find(path);
        return it != endpoints_.end() ? it->second : nullptr;
    }

private:
    void loadEndpoints() {
        std::filesystem::path endpointDir = std::filesystem::current_path() / "endpoints";
        for (const auto& entry : std::filesystem::directory_iterator(endpointDir)) {
            if (entry.path().extension() == ".so" || 
                entry.path().extension() == ".dylib" ||
                entry.path().extension() == ".dll") {
                
                void* handle = dlopen(entry.path().c_str(), RTLD_NOW);
                if (handle) {
                    auto createFunc = (IEndpoint*(*)())dlsym(handle, "createEndpoint");
                    if (createFunc) {
                        auto endpoint = std::shared_ptr<IEndpoint>(createFunc(), [handle](IEndpoint* p) {
                            delete p;
                            dlclose(handle);
                        });
                        auto info = endpoint->getRouteInfo();
                        endpoints_[info.path] = endpoint;
                        lastWriteTimes_[entry.path().string()] = std::filesystem::last_write_time(entry.path());
                    }
                }
            }
        }
    }

    void checkForUpdates() {
        std::filesystem::path endpointDir = std::filesystem::current_path() / "endpoints";
        fmt::print("Checking for updates in: {}\n", endpointDir.string());
        
        std::set<std::string> currentFiles;
        std::map<std::string, std::string> pathToEndpoint;
        
        // Check for new or modified files
        for (const auto& entry : std::filesystem::directory_iterator(endpointDir)) {
            if (entry.path().extension() == ".so" || 
                entry.path().extension() == ".dylib" ||
                entry.path().extension() == ".dll") {
                
                std::string filepath = entry.path().string();
                currentFiles.insert(filepath);
                
                auto currentTime = std::filesystem::last_write_time(entry.path());
                auto it = lastWriteTimes_.find(filepath);
                
                // Force reload if file is new or time changed
                bool needsReload = it == lastWriteTimes_.end() || it->second != currentTime;
                
                if (needsReload) {
                    fmt::print("Loading endpoint: {}\n", filepath);
                    void* handle = dlopen(filepath.c_str(), RTLD_NOW | RTLD_GLOBAL);
                    if (handle) {
                        auto createFunc = (IEndpoint*(*)())dlsym(handle, "createEndpoint");
                        if (createFunc) {
                            auto endpoint = std::shared_ptr<IEndpoint>(createFunc(), [handle](IEndpoint* p) {
                                delete p;
                                dlclose(handle);
                            });
                            auto info = endpoint->getRouteInfo();
                            
                            // Remove old endpoint if it exists
                            endpoints_.erase(info.path);
                            
                            // Add new endpoint
                            endpoints_[info.path] = endpoint;
                            lastWriteTimes_[filepath] = currentTime;
                            pathToEndpoint[filepath] = info.path;
                            fmt::print("Updated endpoint: {} {}\n", info.method, info.path);
                        } else {
                            dlclose(handle);
                            fmt::print("Failed to get createEndpoint function: {}\n", dlerror());
                        }
                    } else {
                        fmt::print("Failed to load library: {}\n", dlerror());
                    }
                }
            }
        }
        
        // Remove deleted endpoints
        for (const auto& [path, _] : lastWriteTimes_) {
            if (currentFiles.find(path) == currentFiles.end()) {
                fmt::print("Endpoint was deleted: {}\n", path);
                if (pathToEndpoint.find(path) != pathToEndpoint.end()) {
                    std::string endpointPath = pathToEndpoint[path];
                    fmt::print("Removing endpoint: {}\n", endpointPath);
                    endpoints_.erase(endpointPath);
                }
                lastWriteTimes_.erase(path);
            }
        }
    }

    std::map<std::string, std::shared_ptr<IEndpoint>> endpoints_;
    std::map<std::string, std::filesystem::file_time_type> lastWriteTimes_;
};

extern "C" EXPORT IRouter* createRouter() {
    return new WebRouter();
} 