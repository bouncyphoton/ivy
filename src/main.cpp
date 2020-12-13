#include "ivy/engine.h"

int main() {
    ivy::Engine engine;
    engine.getOptions().appName = "Ivy Engine Demo";
    engine.getOptions().renderWidth = 800;
    engine.getOptions().renderHeight = 600;

    engine.run();

    return 0;
}
