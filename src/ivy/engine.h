#ifndef IVY_ENGINE_H
#define IVY_ENGINE_H

#include "ivy/types.h"
#include "ivy/platform/platform.h"
#include "ivy/graphics/renderer.h"

namespace ivy {

/**
 * \brief Engine options
 */
struct Options {
    int renderWidth = 1280;
    int renderHeight = 720;
    const char *windowTitle = "ivy";
};

/**
 * \brief The core engine, everything runs from here
 */
class Engine final {
public:
    Engine();
    ~Engine();

    /**
     * \brief Runs the engine
     */
    void run();

    Options &getOptions() {
        return options_;
    }

private:
    Options options_;

    Platform platform_;
    Renderer renderer_;
};

}

#endif // IVY_ENGINE_H
