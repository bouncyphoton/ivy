#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/graphics/render_device.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/vertex.h"
#include "ivy/graphics/geometry.h"
#include "ivy/entity/entity.h"

/**
 * \brief High level renderer
 */
class Renderer final {
public:
    /**
     * \brief Debug rendering mode
     */
    enum class DebugMode {
        FULL, DIFFUSE, NORMAL, WORLD, DEPTH
    };

    explicit Renderer(ivy::gfx::RenderDevice &render_device);
    ~Renderer();

    /**
     * \brief Render a frame
     */
    void render(const std::vector<ivy::Entity> &entities, DebugMode debug_mode = DebugMode::FULL);

private:
    ivy::gfx::RenderDevice &device_;
    std::vector<ivy::gfx::GraphicsPass> passes_;
};

#endif // IVY_RENDERER_H
