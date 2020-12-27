#include "graphics_pass.h"
#include "ivy/graphics/render_device.h"
#include "ivy/graphics/vk_utils.h"
#include "ivy/log.h"

namespace ivy::gfx {

GraphicsPass GraphicsPassBuilder::build() {
    // TODO: input validation/error checking

    Log::debug("Building graphics pass");

    // Create attachments
    std::vector<VkAttachmentDescription> attachments;
    std::unordered_map<std::string, u32> attachmentLocations;
    for (const auto &attachment_pair : attachments_) {
        Log::debug("- Processing attachment: %s", attachment_pair.first.c_str());
        attachmentLocations[attachment_pair.first] = attachments.size();
        attachments.emplace_back(attachment_pair.second.vkAttachmentDescription);
    }

    // Create subpasses
    std::vector<VkSubpassDescription> subpasses;
    std::unordered_map<std::string, u32> subpassLocations;
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> colorAttachmentReferences;
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> inputAttachmentReferences;
    for (const std::string &subpass_name : subpassOrder_) {
        Log::debug("- Processing subpass: %s", subpass_name.c_str());
        subpassLocations[subpass_name] = subpasses.size();

        // Create attachment references
        for (const std::string &inputAttachmentName : subpassDescriptions_[subpass_name].inputAttachmentNames_) {
            Log::debug("  - Processing input attachment: %s", inputAttachmentName.c_str());
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[inputAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Keep track that this attachment was used as an input attachment
            attachments_[inputAttachmentName].usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

            inputAttachmentReferences[subpass_name].emplace_back(reference);
        }
        for (const std::string &colorAttachmentName : subpassDescriptions_[subpass_name].colorAttachmentNames_) {
            Log::debug("  - Processing color attachment: %s", colorAttachmentName.c_str());
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[colorAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Keep track that this attachment was used as a color attachment
            attachments_[colorAttachmentName].usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            colorAttachmentReferences[subpass_name].emplace_back(reference);
        }

        // Create the subpass description
        VkSubpassDescription subpassDescription = {};
        subpassDescription.flags = 0;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.inputAttachmentCount = (u32) inputAttachmentReferences[subpass_name].size();
        subpassDescription.pInputAttachments = inputAttachmentReferences[subpass_name].data();
        subpassDescription.colorAttachmentCount = (u32) colorAttachmentReferences[subpass_name].size();
        subpassDescription.pColorAttachments = colorAttachmentReferences[subpass_name].data();
        subpassDescription.pResolveAttachments = nullptr;
        subpassDescription.pDepthStencilAttachment = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;

        subpasses.emplace_back(subpassDescription);
    }
    subpassLocations[GraphicsPass::SwapchainName] = VK_SUBPASS_EXTERNAL;

    Log::debug("- Building subpass dependencies");

    // Create dependencies
    std::vector<VkSubpassDependency> dependencies = {};
    for (const SubpassDependency &dependencyInfo : subpassDependencies_) {
        Log::debug("  - Processing dependency: %s -> %s", dependencyInfo.srcSubpass.c_str(), dependencyInfo.dstSubpass.c_str());
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

    Log::debug("- Building pipelines");

    // Create graphics pipelines
    std::vector<VkPipeline> subpassPipelines;
    for (const std::string &subpass_name : subpassOrder_) {
        const SubpassDescription &desc = subpassDescriptions_[subpass_name];

        // Create descriptor sets and pipeline layout
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (u32 i = 0; i < desc.inputAttachmentNames_.size(); ++i) {
            bindings.emplace_back();
            bindings.back().binding = i;
            bindings.back().descriptorCount = 1;
            bindings.back().descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        // Debug logging
        Log::debug("  - Subpass %s has %d bindings", subpass_name.c_str(), bindings.size());
        for (u32 i = 0; i < bindings.size(); ++i) {
            Log::debug("    - binding %d: %s", i, vk_descriptor_type_to_string(bindings[i].descriptorType));
        }

        VkPipelineLayout layout = device_.createLayout(bindings);

        subpassPipelines.emplace_back(
            device_.createGraphicsPipeline(
                desc.shaders_, desc.vertexDescription_, layout, renderPass, subpassPipelines.size(),
                desc.colorAttachmentNames_.size()
            )
        );
    }

    return GraphicsPass(renderPass, subpassPipelines, attachments_);
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

    attachments_[attachment_name].vkAttachmentDescription = attachment;
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

SubpassBuilder &SubpassBuilder::addInputAttachment(const std::string &attachment_name) {
    subpass_.inputAttachmentNames_.emplace_back(attachment_name);
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
