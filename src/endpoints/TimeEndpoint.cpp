#include "hot_reload/interfaces.hpp"
#include <fmt/core.h>
#include <chrono>

class TimeEndpoint : public IEndpoint {
public:
    RouteInfo getRouteInfo() const override {
        return {"/time", "GET", "Get current time"};
    }

    std::string handle(std::string_view body) override {
        return fmt::format("🕒 Current time: {}", 
            std::chrono::system_clock::now().time_since_epoch().count());
    }
};

extern "C" EXPORT IEndpoint* createEndpoint() {
    return new TimeEndpoint();
} 