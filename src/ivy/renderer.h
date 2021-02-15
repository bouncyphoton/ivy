#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/graphics/render_device.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/vertex.h"
#include "ivy/graphics/geometry.h"
#include "ivy/entity/entity.h"

namespace ivy {

struct Options;
class Platform;

/**
 * \brief High level renderer
 */
class Renderer final {
public:
    explicit Renderer(gfx::RenderDevice &render_device);
    ~Renderer();

    /**
     * \brief Render a frame
     */
    void render(const std::vector<Entity> &entities);

private:
    gfx::RenderDevice &device_;
    std::vector<gfx::GraphicsPass> passes_;
};

}

#endif // IVY_RENDERER_H
