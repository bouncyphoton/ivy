#include "engine.h"
#include "ivy/log.h"

namespace ivy {

Engine::Engine()
    : platform_(*this), renderer_(platform_) {
    LOG_CHECKPOINT();
}

Engine::~Engine() {
    LOG_CHECKPOINT();
}

void Engine::run() {
    LOG_CHECKPOINT();

    while (!platform_.isCloseRequested()) {
        renderer_.render();

        platform_.update();
    }
}

}
