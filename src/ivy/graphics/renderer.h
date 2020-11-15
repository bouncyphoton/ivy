#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/graphics/vulkan/render_context.h"

namespace ivy {

class Options;
class Platform;

/**
 * \brief High level renderer
 */
class Renderer final {
public:
    Renderer(const Platform &platform);
    ~Renderer();

    /**
     * \brief Render a frame
     */
    void render();

private:
    RenderContext ctx_;
};

}

#endif // IVY_RENDERER_H
