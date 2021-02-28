#ifndef IVY_ENGINE_H
#define IVY_ENGINE_H

#include "ivy/options.h"
#include "ivy/platform/platform.h"
#include "ivy/graphics/render_device.h"
#include "ivy/resources/resource_manager.h"

namespace ivy {

/**
 * \brief The core engine, everything runs from here
 */
class Engine final {
public:
    explicit Engine(const Options &options);
    ~Engine();

    /**
     * \brief Runs the engine
     */
    void run(const std::function<void()> &init_func,
             const std::function<void()> &update_func,
             const std::function<void()> &render_func);

    /**
     * \brief Stops the engine
     */
    void stop();

    Platform &getPlatform() {
        return platform_;
    }

    gfx::RenderDevice &getRenderDevice() {
        return renderDevice_;
    }

    ResourceManager &getResourceManager() {
        return resourceManager_;
    }

private:
    Options options_;
    Platform platform_;
    gfx::RenderDevice renderDevice_;
    ResourceManager resourceManager_;

    bool stopped_ = false;
};

}

#endif // IVY_ENGINE_H
