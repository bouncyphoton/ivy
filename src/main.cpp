#include "ivy/engine.h"

int main() {
    // Static so that destructor is called on exit()
    static ivy::Engine engine;
    engine.run();
}
