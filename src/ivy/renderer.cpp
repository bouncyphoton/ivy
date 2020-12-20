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

    /* Maybe the dream front-facing api?
     * Want: Concise but still powerful
     *
     * pass = passBuilder()
     *        .addShader(VERTEX, ".vert.spv")
     *        .addShader(FRAGMENT, ".frag.spv")
     *        .addVertexDescription(...)
     *        .addDescriptorSet(...)
     *        .addAttachment("albedo", ...)
     *        .addAttachment("roughMetalOcclusion")
     *        .addAttachment("depth", ...)
     *        .addAttachment(SWAPCHAIN_OUT)
     *        .addSubpass("depthprepass", input: {}, color: {}, depthstencil: {"depth"}, resolve: {})
     *        .addSubpass("gbuffer", input: {}, color: {"albedo", "roughMetalOcclusion"})
     *        .addSubpass("lighting", input: {"albedo", "roughMetalOcclusion", "depth"}, color: {SWAPCHAIN_OUT})
     *        .addSubpassDependency("depthprepass" -> "gbuffer", ...)
     *        .addSubpassDependency("gbuffer" -> "lighting", ...)
     *        .build();
     *
     * pass.execute(framebuffer, []() {
     *     ...
     *     cmd.draw();
     *
     *     cmd.nextSubpass();
     *     ...
     *     cmd.draw();
     * });
     */

    // Create attachments
    std::vector<VkAttachmentDescription> colorAttachments;
    std::vector<VkAttachmentReference> colorAttachmentReferences;
    // We need an array of all attachments as well, although currently we have just color
    std::vector<VkAttachmentDescription> &attachments = colorAttachments;
    {
        // We have one attachment that outputs to swapchain
        VkAttachmentDescription color = {};
        color.format = device_.getSwapchainFormat();
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorAttachments.emplace_back(color);

        // Attachment reference
        VkAttachmentReference ref = {};
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ref.attachment = 0;

        colorAttachmentReferences.emplace_back(ref);
    }

    // Create subpasses
    std::vector<VkSubpassDescription> subpasses;
    {
        VkSubpassDescription subpass = {};
        subpass.flags = 0;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.colorAttachmentCount = (u32) colorAttachmentReferences.size();
        subpass.pColorAttachments = colorAttachmentReferences.data();
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        subpasses.emplace_back(subpass);
    }

    // Create subpass dependencies
    std::vector<VkSubpassDependency> subpassDependencies;
    {
        // Before our subpass can write to the image, the swapchain must be done reading from it
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        subpassDependencies.emplace_back(dependency);
    }

    // Create render pass
    renderPass_ = device_.createRenderPass(attachments, subpasses, subpassDependencies);

    // Create graphics pipeline
    {
        // Get shaders
        std::vector<gfx::Shader> shaders;
        shaders.emplace_back(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/triangle.vert.spv");
        shaders.emplace_back(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/triangle.frag.spv");

        // Get vertex description
        gfx::VertexDescription vertexDescription(gfx::VertexP3C3::getBindingDescriptions(),
                                                 gfx::VertexP3C3::getAttributeDescriptions());

        // Create pipeline layout
        VkPipelineLayout layout = device_.createLayout();

        graphicsPipeline_ = device_.createGraphicsPipeline(shaders, vertexDescription, layout, renderPass_);
    }

    // Create vertex buffer
    vertexBuffer_ = device_.createVertexBuffer(vertices, sizeof(vertices[0]) * COUNTOF(vertices));
}

Renderer::~Renderer() {
    LOG_CHECKPOINT();
}

void Renderer::render() {
    device_.beginFrame();
    gfx::CommandBuffer cmd = device_.getCommandBuffer();

    cmd.bindGraphicsPipeline(graphicsPipeline_);
    cmd.executeRenderPass(renderPass_, device_.getSwapchainFramebuffer(renderPass_), [&]() {
        cmd.bindVertexBuffer(vertexBuffer_);
        cmd.draw(COUNTOF(vertices), 1, 0, 0);
    });

    device_.endFrame();
}

}
