#ifndef IVY_TEST_GAME_H
#define IVY_TEST_GAME_H

#include "ivy/engine.h"
#include "renderer_raster.h"
#include "renderer_rt.h"
#include <vector>

class TestGame {
public:
    TestGame();

    void init();

    void update();

    void render();

private:
    static ivy::Options getOptions();

    ivy::Engine engine_;
    RendererRaster rendererRaster_;
    RendererRT rendererRT_;
    ivy::Scene scene_;

    RendererRaster::DebugMode debugMode_ = RendererRaster::DebugMode::FULL;
};

#endif // IVY_TEST_GAME_H
