#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/graphics/vulkan/render_context.h"

namespace ivy {

struct Options;
class Platform;

/**
 * \brief High level renderer
 */
class Renderer final {
public:
    explicit Renderer(const Options &options, const Platform &platform);
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
