#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <map>

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

// Forward declarations
class Controller;
class Router;
class Endpoint;

struct RouteInfo {
    std::string path;
    std::string method;
    std::string description;
};

class EXPORT IEndpoint {
public:
    virtual ~IEndpoint() = default;
    virtual RouteInfo getRouteInfo() const = 0;
    virtual std::string handle(std::string_view body = "") = 0;
};

class EXPORT IRouter {
public:
    virtual ~IRouter() = default;
    virtual std::vector<RouteInfo> getRoutes() const = 0;
    virtual std::shared_ptr<IEndpoint> getEndpoint(const std::string& path) = 0;
};

class EXPORT IController {
public:
    virtual ~IController() = default;
    virtual std::shared_ptr<IRouter> getRouter() = 0;
};

class EXPORT Plugin {
public:
    virtual ~Plugin() = default;
    virtual std::vector<std::shared_ptr<IController>> getControllers() = 0;
    virtual std::string handleRequest(std::string_view path, 
                                    std::string_view method,
                                    std::string_view body = "") = 0;
};

extern "C" EXPORT Plugin* createPlugin(); 