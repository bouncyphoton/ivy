#include "command_buffer.h"
#include "render_device.h"
#include "ivy/consts.h"

namespace ivy::gfx {

void CommandBuffer::bindPipeline(VkPipeline pipeline, VkPipelineBindPoint bind_point) {
    vkCmdBindPipeline(commandBuffer_, bind_point, pipeline);
}

void CommandBuffer::bindGraphicsPipeline(const GraphicsPass &pass, u32 subpass) {
    bindPipeline(pass.getSubpass(subpass).getPipeline(), VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void CommandBuffer::bindComputePipeline(const ComputePass &pass) {
    bindPipeline(pass.getPipeline(), VK_PIPELINE_BIND_POINT_COMPUTE);
}

void CommandBuffer::executeGraphicsPass(RenderDevice &device, const GraphicsPass &pass,
                                        const std::function<void()> &func) {
    Framebuffer &framebuffer = device.getFramebuffer(pass);

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = pass.getVkRenderPass();
    renderPassBeginInfo.framebuffer = framebuffer.getVkFramebuffer();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = pass.getExtent();

    // Generate clear values for each attachment
    std::vector<VkClearValue> clearValues;
    clearValues.reserve(pass.getAttachmentInfos().size());
    for (const auto &infoPair : pass.getAttachmentInfos()) {
        const AttachmentInfo &info = infoPair.second;
        VkClearValue value = {};

        // VkClearValue unions color and depth stencil
        if (info.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            value.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        } else if (info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            value.depthStencil = {1.0f, 0};
        }

        clearValues.emplace_back(value);
    }
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = renderPassBeginInfo.renderArea.extent;

    // Start render pass, call user functions, end render pass
    vkCmdBeginRenderPass(commandBuffer_, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport (flipped upside down)
    setViewport((f32) renderPassBeginInfo.renderArea.offset.x, (f32) renderPassBeginInfo.renderArea.offset.y,
                (f32) renderPassBeginInfo.renderArea.extent.width, (f32) renderPassBeginInfo.renderArea.extent.height,
                0.0f, 1.0f, true);

    vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);
    func();
    vkCmdEndRenderPass(commandBuffer_);
}

void CommandBuffer::executeComputePass(const ComputePass &pass, const std::function<void()> &func) {
    bindComputePipeline(pass);
    func();
}

void CommandBuffer::nextSubpass() {
    vkCmdNextSubpass(commandBuffer_, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::setDescriptorSet(RenderDevice &device, const GraphicsPass &pass, const DescriptorSet &set) {
    if (consts::DEBUG) {
        set.validate();
    }

    VkDescriptorSet vkSet = device.getVkDescriptorSet(pass, set);

    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pass.getSubpass(set.getSubpassIndex()).getPipelineLayout(), set.getSetIndex(),
                            1, &vkSet, 0, nullptr);
}

void CommandBuffer::setDescriptorSet(RenderDevice &device, const ComputePass &pass, const DescriptorSet &set) {
    if (consts::DEBUG) {
        set.validate();
    }

    VkDescriptorSet vkSet = device.getVkDescriptorSet(pass, set);

    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pass.getPipelineLayout(), set.getSetIndex(),
                            1, &vkSet, 0, nullptr);
}

void CommandBuffer::bindVertexBuffer(VkBuffer buffer) {
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &buffer, offsets);
}

void CommandBuffer::bindIndexBuffer(VkBuffer buffer) {
    vkCmdBindIndexBuffer(commandBuffer_, buffer, 0, VK_INDEX_TYPE_UINT32);
}

void CommandBuffer::draw(u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance) {
    vkCmdDraw(commandBuffer_, num_vertices, num_instances, first_vertex, first_instance);
}

void CommandBuffer::drawIndexed(u32 num_indices, u32 num_instances, u32 first_index, u32 vertex_offset,
                                u32 first_instance) {
    vkCmdDrawIndexed(commandBuffer_, num_indices, num_instances, first_index, vertex_offset, first_instance);
}

void CommandBuffer::dispatch(u32 num_groups_x, u32 num_groups_y, u32 num_groups_z) {
    vkCmdDispatch(commandBuffer_, num_groups_x, num_groups_y, num_groups_z);
}

void CommandBuffer::setViewport(f32 x, f32 y, f32 width, f32 height, f32 min_depth, f32 max_depth, bool flip_viewport) {
    VkViewport viewport = {};
    if (flip_viewport) {
        viewport.width = width;
        viewport.height = -1.0f * height;
        viewport.x = x;
        viewport.y = y + height;
        viewport.minDepth = min_depth;
        viewport.maxDepth = max_depth;
    } else {
        viewport.width = width;
        viewport.height = height;
        viewport.x = x;
        viewport.y = y;
        viewport.minDepth = min_depth;
        viewport.maxDepth = max_depth;
    }

    vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
}

void CommandBuffer::copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkDeviceSize dst_offset,
                               VkDeviceSize src_offset) {
    VkBufferCopy region = {};
    region.size = size;
    region.dstOffset = dst_offset;
    region.srcOffset = src_offset;

    vkCmdCopyBuffer(commandBuffer_, src, dst, 1, &region);
}

void CommandBuffer::copyBufferToImage(VkBuffer src, VkImage dst, VkImageLayout dst_layout,
                                      VkImageAspectFlags image_aspect, u32 width, u32 height, u32 depth, u32 layers) {
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = image_aspect;
    region.imageSubresource.layerCount = layers;
    region.imageExtent = {width, height, depth};

    vkCmdCopyBufferToImage(commandBuffer_, src, dst, dst_layout, 1, &region);
}

void CommandBuffer::copyImage(VkImage src, VkImageLayout src_layout, VkImage dst, VkImageLayout dst_layout,
                              u32 num_regions, const VkImageCopy *regions) {
    vkCmdCopyImage(commandBuffer_, src, src_layout, dst, dst_layout, num_regions, regions);
}

void CommandBuffer::clearAttachments(u32 num_attachments, const VkClearAttachment *attachments, u32 num_rects,
                                     const VkClearRect *rects) {
    vkCmdClearAttachments(commandBuffer_, num_attachments, attachments, num_rects, rects);
}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
                                    VkDependencyFlags dependency, u32 num_memory_barriers,
                                    const VkMemoryBarrier *memory_barriers, u32 num_buffer_memory_barriers,
                                    const VkBufferMemoryBarrier *buffer_memory_barriers,
                                    u32 num_image_memory_barriers,
                                    const VkImageMemoryBarrier *image_memory_barriers) {
    vkCmdPipelineBarrier(commandBuffer_, src_stage, dst_stage, dependency,
                         num_memory_barriers, memory_barriers,
                         num_buffer_memory_barriers, buffer_memory_barriers,
                         num_image_memory_barriers, image_memory_barriers);
}

void CommandBuffer::transitionImage(const Texture &texture, VkImageLayout old_layout, VkImageLayout new_layout,
                                    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access) {
    VkImageMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.oldLayout = old_layout;
    memoryBarrier.newLayout = new_layout;
    memoryBarrier.srcAccessMask = src_access;
    memoryBarrier.dstAccessMask = dst_access;
    memoryBarrier.image = texture.getImage();
    memoryBarrier.subresourceRange.aspectMask = texture.getImageAspect();
    memoryBarrier.subresourceRange.levelCount = texture.getLevels();
    memoryBarrier.subresourceRange.layerCount = texture.getLayers();

    pipelineBarrier(src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

}
