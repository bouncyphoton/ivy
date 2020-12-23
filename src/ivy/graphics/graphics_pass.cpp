#include "graphics_pass.h"
#include "ivy/log.h"

namespace ivy::gfx {

GraphicsPass GraphicsPassBuilder::build() {
    // TODO: input validation/error checking

    // Create attachments
    std::vector<VkAttachmentDescription> attachments;
    std::unordered_map<std::string, u32> attachmentLocations;
    for (const auto &attachment_pair : attachments_) {
        Log::debug("Processing attachment: %s", attachment_pair.first.c_str());
        attachmentLocations[attachment_pair.first] = attachments.size();
        attachments.emplace_back(attachment_pair.second);
    }

    // Create subpasses
    std::vector<VkSubpassDescription> subpasses;
    std::unordered_map<std::string, u32> subpassLocations;
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> colorAttachmentReferences;
    for (const std::string &subpass_name : subpassOrder_) {
        Log::debug("Processing subpass: %s", subpass_name.c_str());
        subpassLocations[subpass_name] = subpasses.size();

        // Create attachment references
        for (const std::string &colorAttachmentNames : subpassDescriptions_[subpass_name].colorAttachmentNames_) {
            Log::debug("Processing color attachment: %s", colorAttachmentNames.c_str());
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[colorAttachmentNames];
            reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            colorAttachmentReferences[subpass_name].emplace_back(reference);
        }

        // Create the subpass description
        VkSubpassDescription subpassDescription = {};
        subpassDescription.flags = 0;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.colorAttachmentCount = (u32) colorAttachmentReferences[subpass_name].size();
        subpassDescription.pColorAttachments = colorAttachmentReferences[subpass_name].data();
        subpassDescription.pResolveAttachments = nullptr;
        subpassDescription.pDepthStencilAttachment = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;

        subpasses.emplace_back(subpassDescription);
    }
    subpassLocations[GraphicsPass::SwapchainName] = VK_SUBPASS_EXTERNAL;

    // Create dependencies
    std::vector<VkSubpassDependency> dependencies = {};
    for (const SubpassDependency &dependencyInfo : subpassDependencies_) {
        Log::debug("Processing dependency: %s -> %s", dependencyInfo.srcSubpass.c_str(), dependencyInfo.dstSubpass.c_str());
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = subpassLocations[dependencyInfo.srcSubpass];
        dependency.dstSubpass = subpassLocations[dependencyInfo.dstSubpass];
        dependency.srcStageMask = dependencyInfo.srcStageMask;
        dependency.dstStageMask = dependencyInfo.dstStageMask;
        dependency.srcAccessMask = dependencyInfo.srcAccessFlags;
        dependency.dstAccessMask = dependencyInfo.dstAccessFlags;

        dependencies.emplace_back(dependency);
    }

    // Create render pass
    VkRenderPass renderPass = device_.createRenderPass(attachments, subpasses, dependencies);

    // Create pipeline layout
    VkPipelineLayout layout = device_.createLayout();

    // Create graphics pipelines
    std::vector<VkPipeline> subpassPipelines;
    for (const std::string &subpass_name : subpassOrder_) {
        const SubpassDescription &desc = subpassDescriptions_[subpass_name];

        subpassPipelines.emplace_back(
            device_.createGraphicsPipeline(
                desc.shaders_, desc.vertexDescription_, layout, renderPass, subpassPipelines.size()
            )
        );
    }

    return GraphicsPass(renderPass, subpassPipelines);
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachment(const std::string &attachment_name, VkFormat format,
                                                        VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op,
                                                        VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op,
                                                        VkImageLayout initial_layout, VkImageLayout final_layout) {
    VkAttachmentDescription attachment = {};
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = load_op;
    attachment.storeOp = store_op;
    attachment.stencilLoadOp = stencil_load_op;
    attachment.stencilStoreOp = stencil_store_op;
    attachment.initialLayout = initial_layout;
    attachment.finalLayout = final_layout;

    attachments_[attachment_name] = attachment;
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachmentSwapchain() {
    addAttachment(GraphicsPass::SwapchainName, device_.getSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR,
                  VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addSubpass(const std::string &subpass_name,
                                                     const SubpassDescription &subpass) {
    subpassDescriptions_[subpass_name] = subpass;
    subpassOrder_.emplace_back(subpass_name);
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addSubpassDependency(const std::string &src_subpass_name,
                                                               const std::string &dst_subpass_name,
                                                               VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                                               VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags) {
    SubpassDependency dependency = {};
    dependency.srcSubpass = src_subpass_name;
    dependency.dstSubpass = dst_subpass_name;
    dependency.srcStageMask = src_stage_mask;
    dependency.dstStageMask = dst_stage_mask;
    dependency.srcAccessFlags = src_access_flags;
    dependency.dstAccessFlags = dst_access_flags;

    subpassDependencies_.emplace_back(dependency);
    return *this;
}

SubpassBuilder &SubpassBuilder::addShader(Shader::StageEnum shader_stage, const std::string &shader_path) {
    subpass_.shaders_.emplace_back(shader_stage, shader_path);
    return *this;
}

SubpassBuilder &SubpassBuilder::addVertexDescription(const std::vector<VkVertexInputBindingDescription> &bindings,
                                                     const std::vector<VkVertexInputAttributeDescription> &attributes) {
    // TODO: support multiple vertex descriptions
    subpass_.vertexDescription_ = VertexDescription(bindings, attributes);
    return *this;
}

SubpassBuilder &SubpassBuilder::addColorAttachment(const std::string &attachment_name) {
    subpass_.colorAttachmentNames_.emplace_back(attachment_name);
    return *this;
}

SubpassDescription SubpassBuilder::build() {
    return subpass_;
}

}
