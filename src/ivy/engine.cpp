#include "engine.h"
#include "ivy/log.h"

namespace ivy {

Engine::Engine(const Options &options)
    : options_(options), platform_(options_), renderDevice_(options, platform_),
      resourceManager_(renderDevice_, "../assets") {
    LOG_CHECKPOINT();
}

Engine::~Engine() {
    LOG_CHECKPOINT();
}

void Engine::run(const std::function<void()> &init_func, const std::function<void()> &update_func,
                 const std::function<void()> &render_func) {
    LOG_CHECKPOINT();

    init_func();

    // Main loop
    while (!platform_.isCloseRequested() && !stopped_) {
        // Poll events
        platform_.update();

        // Update and render game
        update_func();
        render_func();
    }
}

void Engine::stop() {
    stopped_ = true;
}

}
