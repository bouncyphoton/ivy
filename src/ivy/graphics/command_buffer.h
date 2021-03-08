#ifndef IVY_COMMAND_BUFFER_H
#define IVY_COMMAND_BUFFER_H

#include "ivy/types.h"
#include "ivy/graphics/framebuffer.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/descriptor_set.h"
#include <vulkan/vulkan.h>
#include <functional>

namespace ivy::gfx {

/**
 * \brief Wrapper around Vulkan command buffer
 */
class CommandBuffer {
public:
    explicit CommandBuffer(VkCommandBuffer command_buffer)
        : commandBuffer_(command_buffer) {}

    void bindGraphicsPipeline(VkPipeline pipeline);

    void bindGraphicsPipeline(const GraphicsPass &pass, u32 subpass);

    void executeGraphicsPass(RenderDevice &device, const GraphicsPass &pass, const std::function<void()> &func);

    void nextSubpass();

    void setDescriptorSet(RenderDevice &device, const GraphicsPass &pass, const DescriptorSet &set);

    void bindVertexBuffer(VkBuffer buffer);

    void bindIndexBuffer(VkBuffer buffer);

    void draw(u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance);

    void drawIndexed(u32 num_indices, u32 num_instances, u32 first_index, u32 vertex_offset, u32 first_instance);

    void copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkDeviceSize dst_offset = 0,
                    VkDeviceSize src_offset = 0);

    void copyBufferToImage(VkBuffer src, VkImage dst, VkImageLayout dst_layout, VkImageAspectFlags image_aspect,
                           u32 width, u32 height, u32 depth = 1);

    void pipelineBarrier(VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkDependencyFlags dependency,
                         u32 num_memory_barriers, const VkMemoryBarrier *memory_barriers,
                         u32 num_buffer_memory_barriers, const VkBufferMemoryBarrier *buffer_memory_barriers,
                         u32 num_image_memory_barriers, const VkImageMemoryBarrier *image_memory_barriers);

private:
    VkCommandBuffer commandBuffer_;
};

}

#endif // IVY_COMMAND_BUFFER_H
