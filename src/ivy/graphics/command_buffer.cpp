#include "command_buffer.h"
#include "render_device.h"
#include "ivy/consts.h"

namespace ivy::gfx {

void CommandBuffer::bindGraphicsPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandBuffer::bindGraphicsPipeline(const GraphicsPass &pass, u32 subpass) {
    bindGraphicsPipeline(pass.getSubpass(subpass).getPipeline());
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

    // Flip the viewport upside down
    VkViewport viewport = {};
    viewport.width = (f32) renderPassBeginInfo.renderArea.extent.width;
    viewport.height = -1.0f * (f32) renderPassBeginInfo.renderArea.extent.height;
    viewport.x = (f32) renderPassBeginInfo.renderArea.offset.x;
    viewport.y = (f32) renderPassBeginInfo.renderArea.offset.y + (f32) renderPassBeginInfo.renderArea.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = renderPassBeginInfo.renderArea.extent;

    // Start render pass, call user functions, end render pass
    vkCmdBeginRenderPass(commandBuffer_, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);
    func();
    vkCmdEndRenderPass(commandBuffer_);
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

void CommandBuffer::copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkDeviceSize dst_offset,
                               VkDeviceSize src_offset) {
    VkBufferCopy region = {};
    region.size = size;
    region.dstOffset = dst_offset;
    region.srcOffset = src_offset;

    vkCmdCopyBuffer(commandBuffer_, src, dst, 1, &region);
}

void CommandBuffer::copyBufferToImage(VkBuffer src, VkImage dst, VkImageLayout dst_layout,
                                      VkImageAspectFlags image_aspect, u32 width, u32 height, u32 depth) {
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = image_aspect;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, depth};

    vkCmdCopyBufferToImage(commandBuffer_, src, dst, dst_layout, 1, &region);
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

}
