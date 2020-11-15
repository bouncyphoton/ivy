#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/platform/platform.h"

namespace ivy {

/**
 * \brief High level renderer
 */
class Renderer final {
public:
    explicit Renderer(const Platform &platform);
    ~Renderer();

    void render();
};

}

#endif // IVY_RENDERER_H
