#include "plugin.hpp"

class HelloEndpoint : public IEndpoint {
public:
    RouteInfo getRouteInfo() const override {
        return {"/hello", "GET", "Get greeting"};
    }

    std::string handle(std::string_view body) override {
        return "👋 Hello from hot-reloaded endpoint!";
    }
};

extern "C" EXPORT IEndpoint* createEndpoint() {
    return new HelloEndpoint();
} 