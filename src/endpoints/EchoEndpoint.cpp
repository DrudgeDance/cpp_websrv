#include "plugin.hpp"
#include <fmt/core.h>

class EchoEndpoint : public IEndpoint {
public:
    RouteInfo getRouteInfo() const override {
        return {"/echo", "POST", "Echo back the request body"};
    }

    std::string handle(std::string_view body) override {
        return fmt::format("📢 Echo: {}", body);
    }
};

extern "C" EXPORT IEndpoint* createEndpoint() {
    return new EchoEndpoint();
} 