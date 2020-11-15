#ifndef IVY_ENGINE_H
#define IVY_ENGINE_H

#include "ivy/types.h"
#include "ivy/platform/platform.h"
#include "ivy/graphics/renderer.h"

namespace ivy {

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
    [[noreturn]] void run();

private:
    Platform platform_;
    Renderer renderer_;
};

}

#endif // IVY_ENGINE_H
