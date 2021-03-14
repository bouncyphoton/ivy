#ifndef IVY_RENDERER_H
#define IVY_RENDERER_H

#include "ivy/types.h"
#include "ivy/graphics/render_device.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/vertex.h"
#include "ivy/graphics/geometry.h"
#include "ivy/scene/scene.h"

/**
 * \brief High level renderer
 */
class Renderer final {
public:
    /**
     * \brief Debug rendering mode
     */
    enum class DebugMode {
        FULL, DIFFUSE, NORMAL, WORLD, SHADOW_MAP
    };

    explicit Renderer(ivy::gfx::RenderDevice &render_device);
    ~Renderer();

    /**
     * \brief Render a frame
     */
    void render(ivy::Scene &scene, DebugMode debug_mode = DebugMode::FULL);

private:
    [[nodiscard]] glm::vec4 getShadowViewport(ivy::u32 shadow_idx) const;

    ivy::gfx::RenderDevice &device_;
    std::vector<ivy::gfx::GraphicsPass> passes_;

    const ivy::u32 shadowMapSize_ = 2048;
    ivy::u32 numShadows = 0;
    ivy::u32 shadowsPerSide = 0;
    ivy::u32 shadowSize = 0;
    VkSampler shadowSampler_;
};

#endif // IVY_RENDERER_H
