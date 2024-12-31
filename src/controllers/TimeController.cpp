#include "plugin.hpp"
#include <memory>
#include <dlfcn.h>

class TimeController : public IController {
public:
    std::shared_ptr<IRouter> getRouter() override {
        if (!router_) {
            void* handle = dlopen("libApiRouter.dylib", RTLD_NOW);
            if (handle) {
                auto createFunc = (IRouter*(*)())dlsym(handle, "createRouter");
                if (createFunc) {
                    router_ = std::shared_ptr<IRouter>(createFunc());
                }
            }
        }
        return router_;
    }

private:
    std::shared_ptr<IRouter> router_;
};

extern "C" EXPORT IController* createController() {
    return new TimeController();
} 