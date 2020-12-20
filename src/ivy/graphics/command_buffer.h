#ifndef IVY_COMMAND_BUFFER_H
#define IVY_COMMAND_BUFFER_H

#include "ivy/types.h"
#include "ivy/graphics/framebuffer.h"
#include <vulkan/vulkan.h>
#include <functional>

namespace ivy::gfx {

/**
 * \brief Wrapper around Vulkan command buffer
 */
class CommandBuffer {
public:
    void bindGraphicsPipeline(VkPipeline pipeline);

    void executeRenderPass(VkRenderPass render_pass, Framebuffer framebuffer, const std::function<void()> &func);

    void draw(u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance);

private:
    friend class RenderDevice;

    explicit CommandBuffer(VkCommandBuffer command_buffer)
        : commandBuffer_(command_buffer) {}

    VkCommandBuffer commandBuffer_;
};

}

#endif // IVY_COMMAND_BUFFER_H
