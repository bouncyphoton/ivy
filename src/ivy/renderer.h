#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/graphics/render_device.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/vertex.h"
#include "ivy/graphics/mesh.h"
#include "ivy/entity/entity.h"

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

    // TODO: remove temp entities in renderer
    std::vector<Entity> entities_;

    gfx::MeshStatic<gfx::VertexP3C3> mesh_;
};

}

#endif // IVY_RENDERER_H
