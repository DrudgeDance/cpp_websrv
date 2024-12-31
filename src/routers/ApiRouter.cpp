#include "plugin.hpp"
#include <filesystem>
#include <dlfcn.h>

class ApiRouter : public IRouter {
public:
    ApiRouter() {
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
        auto it = endpoints_.find(path);
        return it != endpoints_.end() ? it->second : nullptr;
    }

private:
    void loadEndpoints() {
        std::filesystem::path endpointDir = "endpoints";
        for (const auto& entry : std::filesystem::directory_iterator(endpointDir)) {
            if (entry.path().extension() == ".so" || 
                entry.path().extension() == ".dylib" ||
                entry.path().extension() == ".dll") {
                
                void* handle = dlopen(entry.path().c_str(), RTLD_NOW);
                if (handle) {
                    auto createFunc = (IEndpoint*(*)())dlsym(handle, "createEndpoint");
                    if (createFunc) {
                        auto endpoint = std::shared_ptr<IEndpoint>(createFunc());
                        auto info = endpoint->getRouteInfo();
                        endpoints_[info.path] = endpoint;
                    }
                }
            }
        }
    }

    std::map<std::string, std::shared_ptr<IEndpoint>> endpoints_;
};

extern "C" EXPORT IRouter* createRouter() {
    return new ApiRouter();
} 