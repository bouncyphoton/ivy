#include "engine.h"
#include "ivy/log.h"

namespace ivy {

Engine::Engine()
    : renderer_(platform_) {
    LOG_CHECKPOINT();
}

Engine::~Engine() {
    LOG_CHECKPOINT();
}

void Engine::run() {
    LOG_CHECKPOINT();

    for (;;) {
        platform_.update();

        renderer_.render();
    }
}

}
