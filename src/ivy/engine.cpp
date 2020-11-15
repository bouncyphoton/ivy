#include "engine.h"
#include "ivy/log.h"
#include "ivy/platform/platform.h"
#include "ivy/graphics/renderer.h"

namespace ivy {

Engine::Engine() {
    LOG_CHECKPOINT();
}

Engine::~Engine() {
    LOG_CHECKPOINT();
}

void Engine::run() {
    LOG_CHECKPOINT();

    // These are static so that fatal error still calls destructors
    static Platform platform(options_);
    static Renderer renderer(platform);

    // Main loop
    while (!platform.isCloseRequested()) {
        platform.update();

        renderer.render();
    }
}

}
