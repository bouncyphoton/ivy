#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"

namespace ivy {

// TODO: remove temp vertices
gfx::VertexP3C3 vertices[] = {
    gfx::VertexP3C3({0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
    gfx::VertexP3C3({0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
    gfx::VertexP3C3({-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f})
};

Renderer::Renderer(const Options &options, const Platform &platform)
    : device_(options, platform) {
    LOG_CHECKPOINT();

    // TODO: find out of this will work with more complicated subpass setups c:

    // Build our graphics pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .addAttachmentSwapchain()
        .addSubpass("main_subpass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/triangle.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/triangle.frag.spv")
                    .addVertexDescription(gfx::VertexP3C3::getBindingDescriptions(), gfx::VertexP3C3::getAttributeDescriptions())
                    .addColorAttachment(gfx::GraphicsPass::SwapchainName)
                    .build()
                   )
        .addSubpassDependency(gfx::GraphicsPass::SwapchainName, "main_subpass", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        .build()
    );

    // Create vertex buffer
    vertexBuffer_ = device_.createVertexBuffer(vertices, sizeof(vertices[0]) * COUNTOF(vertices));
}

Renderer::~Renderer() {
    LOG_CHECKPOINT();
}

void Renderer::render() {
    device_.beginFrame();
    gfx::CommandBuffer cmd = device_.getCommandBuffer();

    gfx::GraphicsPass pass = passes_.front();

    cmd.executeRenderPass(pass.getVkRenderPass(), device_.getSwapchainFramebuffer(pass.getVkRenderPass()), [&]() {
        // Subpass 0
        cmd.bindGraphicsPipeline(pass.getPipelines()[0]);
        cmd.bindVertexBuffer(vertexBuffer_);
        cmd.draw(COUNTOF(vertices), 1, 0, 0);


    });

    device_.endFrame();
}

}
