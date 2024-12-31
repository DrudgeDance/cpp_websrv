#pragma once
#include <string>
#include <vector>
#include <string_view>

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

struct Endpoint {
    std::string path;
    std::string method;  // "GET", "POST", etc.
    std::string description;
};

class EXPORT Plugin {
public:
    virtual ~Plugin() = default;
    
    // Get list of endpoints this plugin provides
    virtual std::vector<Endpoint> getEndpoints() const = 0;
    
    // Handle a request to a specific endpoint
    virtual std::string handleRequest(std::string_view path, 
                                    std::string_view method,
                                    std::string_view body = "") = 0;
};

extern "C" EXPORT Plugin* createPlugin(); 