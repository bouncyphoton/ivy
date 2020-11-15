#include "ivy/engine.h"

int main() {
    // Static so that destructor is called on fatal error exit()
    static ivy::Engine engine;
    engine.run();

    return 0;
}
