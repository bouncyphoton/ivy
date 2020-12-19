#include "renderer.h"
#include "ivy/log.h"

namespace ivy {

// TODO: remove temp vertex
struct Vertex {
    // TODO: glm
    f32 p1, p2, p3;
    f32 c1, c2, c3;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributes;

        // pos
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 0;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(Vertex, p1);

        // color
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 1;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(Vertex, c1);

        return attributes;
    }
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
     *     // ???
     *     // How to handle subpasses?
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

        // an integer value identifying an attachment at the corresponding index in VkRenderPassCreateInfo::pAttachments
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
        dependency.dependencyFlags = 0;

        subpassDependencies.emplace_back(dependency);
    }

    // Create render pass
    VkRenderPass renderPass = device_.createRenderPass(attachments, subpasses, subpassDependencies);

    // Create pipeline layout
    VkPipelineLayout layout = device_.createLayout();

    // Create graphics pipeline
    VkPipeline graphicsPipeline;
    {
        std::vector<gfx::Shader> shaders;
        shaders.emplace_back(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/triangle.vert.spv");
        shaders.emplace_back(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/triangle.frag.spv");

        gfx::VertexDescription vertexDescription(Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());

        graphicsPipeline = device_.createGraphicsPipeline(shaders, vertexDescription, layout, renderPass);
    }

    (void)graphicsPipeline;

    // TODO: framebuffer
}

Renderer::~Renderer() {
    LOG_CHECKPOINT();
    // TODO
}

void Renderer::render() {
    // TODO
}

}
