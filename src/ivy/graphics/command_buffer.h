#ifndef IVY_COMMAND_BUFFER_H
#define IVY_COMMAND_BUFFER_H

#include "ivy/types.h"
#include "ivy/graphics/framebuffer.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/compute_pass.h"
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

    void bindPipeline(VkPipeline pipeline, VkPipelineBindPoint bind_point);

    void bindGraphicsPipeline(const GraphicsPass &pass, u32 subpass);

    void bindComputePipeline(const ComputePass &pass);

    /**
     * \brief Starts the render pass with clear values and correct framebuffer, sets a flipped viewport and
     * scissor region, runs func and ends the render pass. NOTE: this does not bind any pipelines
     * \param device The render device
     * \param pass The graphics pass to execute
     * \param func The function to call after starting the render pass
     */
    void executeGraphicsPass(RenderDevice &device, const GraphicsPass &pass, const std::function<void()> &func);

    /**
     * \brief Binds the compute pipeline and runs func
     * \param pass The compute pass
     * \param func The function to run after binding compute pipeline
     */
    void executeComputePass(const ComputePass &pass, const std::function<void()> &func);

    void nextSubpass();

    void setDescriptorSet(RenderDevice &device, const GraphicsPass &pass, const DescriptorSet &set);

    void setDescriptorSet(RenderDevice &device, const ComputePass &pass, const DescriptorSet &set);

    void bindVertexBuffer(VkBuffer buffer);

    void bindIndexBuffer(VkBuffer buffer);

    void draw(u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance);

    void drawIndexed(u32 num_indices, u32 num_instances, u32 first_index, u32 vertex_offset, u32 first_instance);

    void dispatch(u32 num_groups_x, u32 num_groups_y = 1, u32 num_groups_z = 1);

    void setViewport(f32 x, f32 y, f32 width, f32 height, f32 min_depth = 0.0f, f32 max_depth = 1.0f,
                     bool flip_viewport = false);

    // TODO: setScissor

    void copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkDeviceSize dst_offset = 0,
                    VkDeviceSize src_offset = 0);

    void copyBufferToImage(VkBuffer src, VkImage dst, VkImageLayout dst_layout, VkImageAspectFlags image_aspect,
                           u32 width, u32 height, u32 depth, u32 layers);

    void copyImage(VkImage src, VkImageLayout src_layout, VkImage dst, VkImageLayout dst_layout, u32 num_regions,
                   const VkImageCopy *regions);

    void clearAttachments(u32 num_attachments, const VkClearAttachment *attachments, u32 num_rects,
                          const VkClearRect *rects);

    void pipelineBarrier(VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkDependencyFlags dependency,
                         u32 num_memory_barriers, const VkMemoryBarrier *memory_barriers,
                         u32 num_buffer_memory_barriers, const VkBufferMemoryBarrier *buffer_memory_barriers,
                         u32 num_image_memory_barriers, const VkImageMemoryBarrier *image_memory_barriers);

    void transitionImage(const Texture &texture, VkImageLayout old_layout, VkImageLayout new_layout,
                         VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access);

private:
    VkCommandBuffer commandBuffer_;
};

}

#endif // IVY_COMMAND_BUFFER_H
