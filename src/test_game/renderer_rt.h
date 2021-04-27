#ifndef IVY_RENDERER_RT_H
#define IVY_RENDERER_RT_H

#include "ivy/types.h"
#include "ivy/graphics/render_device.h"
#include "ivy/graphics/compute_pass.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/scene/scene.h"

class RendererRT final {
public:
    explicit RendererRT(ivy::gfx::RenderDevice &render_device);

    void render(ivy::Scene &scene);

private:
    ivy::gfx::RenderDevice &device_;
    VkSampler sampler_;
    std::optional<ivy::gfx::Texture> texture_{};
    std::optional<ivy::gfx::ComputePass> rtPass_{};
    std::optional<ivy::gfx::GraphicsPass> presentPass_{};
};

#endif // IVY_RENDERER_RT_H
