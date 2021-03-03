#ifndef IVY_TEST_GAME_H
#define IVY_TEST_GAME_H

#include "ivy/engine.h"
#include "renderer.h"
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
    Renderer renderer_;
    std::vector<ivy::Entity> entities_;
};

#endif // IVY_TEST_GAME_H