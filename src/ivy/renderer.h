#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/graphics/render_device.h"
#include "ivy/graphics/graphics_pass.h"

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
    gfx::RenderDevice device_;
    std::vector<gfx::GraphicsPass> passes_;
    VkBuffer vertexBuffer_;
};

}

#endif // IVY_RENDERER_H
