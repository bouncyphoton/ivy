#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"

namespace ivy {

// TODO: depth buffering
// TODO: ubo
// TODO: multiple render passes

// TODO: remove temp vertices
gfx::VertexP3C3 vertices[] = {
    gfx::VertexP3C3({0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
    gfx::VertexP3C3({0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
    gfx::VertexP3C3({-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f})
};

Renderer::Renderer(const Options &options, const Platform &platform)
    : device_(options, platform) {
    LOG_CHECKPOINT();

    // Build our graphics pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .addAttachmentSwapchain()
        .addAttachment("albedo", VK_FORMAT_R8G8B8A8_UNORM)
        .addAttachment("position", VK_FORMAT_R16G16B16A16_SFLOAT)
        .addSubpass("gbuffer_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/gbuffer.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/gbuffer.frag.spv")
                    .addVertexDescription(gfx::VertexP3C3::getBindingDescriptions(), gfx::VertexP3C3::getAttributeDescriptions())
                    .addColorAttachment("albedo")
                    .addColorAttachment("position")
                    .build()
                   )
        .addSubpass("lighting_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/lighting.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/lighting.frag.spv")
                    .addInputAttachment("albedo")
                    .addInputAttachment("position")
                    .addColorAttachment(gfx::GraphicsPass::SwapchainName)
                    .build()
                   )
        .addSubpassDependency(gfx::GraphicsPass::SwapchainName, "gbuffer_pass", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        .addSubpassDependency("gbuffer_pass", "lighting_pass", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
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

    cmd.executeGraphicsPass(pass, device_.getFramebuffer(pass), [&]() {
        // TODO: allocate descriptor sets and bind

        // Subpass 0
        cmd.bindGraphicsPipeline(pass.getPipelines()[0]);
        cmd.bindVertexBuffer(vertexBuffer_);
        cmd.draw(COUNTOF(vertices), 1, 0, 0);

        // Subpass 1
        cmd.nextSubpass();
        cmd.bindGraphicsPipeline(pass.getPipelines()[1]);
        cmd.draw(3, 1, 0, 0); // Fullscreen triangle
    });

    device_.endFrame();

    Log::fatal("DEBUG END OF FRAME");
}

}
