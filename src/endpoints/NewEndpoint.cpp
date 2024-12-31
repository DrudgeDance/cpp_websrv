#include "plugin.hpp"

class NewEndpoint : public IEndpoint {
public:
    RouteInfo getRouteInfo() const override {
        return {"/new", "GET", "New hot-reloaded endpoint"};
    }

    std::string handle(std::string_view body) override {
        return "🆕 This endpoint was added via hot reload!";
    }
};

extern "C" EXPORT IEndpoint* createEndpoint() {
    return new NewEndpoint();
} 