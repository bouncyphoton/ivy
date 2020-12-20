#include "command_buffer.h"

namespace ivy::gfx {

void CommandBuffer::bindGraphicsPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandBuffer::executeRenderPass(VkRenderPass render_pass, Framebuffer framebuffer,
                                      const std::function<void()> &func) {
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = render_pass;
    renderPassBeginInfo.framebuffer = framebuffer.getVkFramebuffer();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = framebuffer.getExtent();

    VkClearValue clearColor = {};
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;
    clearColor.depthStencil = {0.0f, 0};

    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer_, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    func();

    vkCmdEndRenderPass(commandBuffer_);
}

void CommandBuffer::bindVertexBuffer(VkBuffer buffer) {
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &buffer, offsets);
}

void CommandBuffer::draw(u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance) {
    vkCmdDraw(commandBuffer_, num_vertices, num_instances, first_vertex, first_instance);
}

void CommandBuffer::copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkDeviceSize dst_offset,
                               VkDeviceSize src_offset) {
    VkBufferCopy region = {};
    region.size = size;
    region.dstOffset = dst_offset;
    region.srcOffset = src_offset;

    vkCmdCopyBuffer(commandBuffer_, src, dst, 1, &region);
}

}
