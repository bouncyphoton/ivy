#include "engine.h"

namespace ivy {

Engine::Engine()
    : renderer_(platform_) {
}

void Engine::run() {
    for (;;) {
        platform_.update();

        renderer_.render();
    }
}

}