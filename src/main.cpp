#include "ivy/engine.h"

int main() {
    ivy::Engine engine;
    engine.getOptions().appName = "Vulkan G-Buffer Demo | ivy engine";
    engine.getOptions().renderWidth = 800;
    engine.getOptions().renderHeight = 600;

    engine.run();

    return 0;
}
