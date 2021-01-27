#include "graphics_pass.h"
#include "ivy/graphics/render_device.h"
#include "ivy/graphics/vk_utils.h"
#include "ivy/log.h"

namespace ivy::gfx {

GraphicsPass GraphicsPassBuilder::build() {
    // TODO: input validation/error checking, swapchain attachments etc.

    Log::debug("Building graphics pass");

    // Create attachmentDescriptions
    std::vector<VkAttachmentDescription> attachmentDescriptions;
    std::unordered_map<std::string, u32> attachmentLocations;
    for (const auto &attachmentPair : attachments_) {
        const std::string &name = attachmentPair.first;
        const AttachmentInfo &info = attachmentPair.second;
        Log::debug("- Processing attachment: %s", name.c_str());

        attachmentLocations[name] = attachmentDescriptions.size();
        attachmentDescriptions.emplace_back(info.description);
    }

    // Create subpasses
    std::vector<VkSubpassDescription> subpassDescriptions;
    std::unordered_map<std::string, u32> subpassLocations;
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> colorAttachmentReferences;
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> inputAttachmentReferences;
    for (const std::string &subpassName : subpassOrder_) {
        Log::debug("- Processing subpass: %s", subpassName.c_str());
        subpassLocations[subpassName] = subpassDescriptions.size();

        // Create attachment references
        for (const std::string &inputAttachmentName : subpassInfos_[subpassName].inputAttachmentNames_) {
            Log::debug("  - Processing input attachment: %s", inputAttachmentName.c_str());
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[inputAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Keep track that this attachment was used as an input attachment
            attachments_[inputAttachmentName].usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

            inputAttachmentReferences[subpassName].emplace_back(reference);
        }
        for (const std::string &colorAttachmentName : subpassInfos_[subpassName].colorAttachmentNames_) {
            Log::debug("  - Processing color attachment: %s", colorAttachmentName.c_str());
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[colorAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Keep track that this attachment was used as a color attachment
            attachments_[colorAttachmentName].usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            colorAttachmentReferences[subpassName].emplace_back(reference);
        }

        // Create the subpass description
        VkSubpassDescription subpassDescription = {};
        subpassDescription.flags = 0;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.inputAttachmentCount = (u32) inputAttachmentReferences[subpassName].size();
        subpassDescription.pInputAttachments = inputAttachmentReferences[subpassName].data();
        subpassDescription.colorAttachmentCount = (u32) colorAttachmentReferences[subpassName].size();
        subpassDescription.pColorAttachments = colorAttachmentReferences[subpassName].data();
        subpassDescription.pResolveAttachments = nullptr;
        subpassDescription.pDepthStencilAttachment = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;

        subpassDescriptions.emplace_back(subpassDescription);
    }
    subpassLocations[GraphicsPass::SwapchainName] = VK_SUBPASS_EXTERNAL;

    Log::debug("- Building subpass dependencies");

    // Create dependencies
    std::vector<VkSubpassDependency> dependencies = {};
    for (const DependencyInfo &info : subpassDependencyInfo_) {
        Log::debug("  - Processing dependency: %s -> %s", info.srcSubpass.c_str(), info.dstSubpass.c_str());
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = subpassLocations.at(info.srcSubpass);
        dependency.dstSubpass = subpassLocations.at(info.dstSubpass);
        dependency.srcStageMask = info.srcStageMask;
        dependency.dstStageMask = info.dstStageMask;
        dependency.srcAccessMask = info.srcAccessFlags;
        dependency.dstAccessMask = info.dstAccessFlags;

        dependencies.emplace_back(dependency);
    }

    // Create render pass
    VkRenderPass renderPass = device_.createRenderPass(attachmentDescriptions, subpassDescriptions, dependencies);

    Log::debug("- Building pipelines");

    // Create subpass pipelines and descriptor set layouts
    std::vector<Subpass> subpasses;
    std::unordered_map<u32, std::unordered_map<u32, DescriptorSetLayout>> descriptorSetLayouts;
    for (u32 subpassIdx = 0; subpassIdx < subpassOrder_.size(); ++subpassIdx) {
        const std::string &subpass_name = subpassOrder_[subpassIdx];
        const SubpassInfo &subpassInfo = subpassInfos_[subpass_name];

        // Descriptor set layout storing
        Log::debug("  - Subpass %s has %d set%s", subpass_name.c_str(), subpassInfo.descriptors_.size(),
                   subpassInfo.descriptors_.size() != 1 ? "s" : "");
        for (const auto &descriptorSet : subpassInfo.descriptors_) {
            u32 setIdx = descriptorSet.first;
            const std::unordered_map<u32, VkDescriptorSetLayoutBinding> &bindings = descriptorSet.second;
            Log::debug("    - set %d has %d bindings", setIdx, bindings.size());

            std::vector<VkDescriptorSetLayoutBinding> bindingsVector;
            for (const auto &binding : bindings) {
                Log::debug("      - binding %d: %s", binding.first, vk_descriptor_type_to_string(binding.second.descriptorType));
                bindingsVector.emplace_back(binding.second);
            }

            // Store descriptor set layout
            descriptorSetLayouts[subpassIdx].emplace(setIdx, DescriptorSetLayout(subpassIdx, setIdx, bindingsVector));
        }

        // Create layout
        SubpassLayout layout = device_.createLayout(subpassInfo.descriptors_);

        // Create pipeline
        subpasses.emplace_back(
            device_.createGraphicsPipeline(
                subpassInfo.shaders_, subpassInfo.vertexDescription_, layout.pipelineLayout, renderPass, subpasses.size(),
                subpassInfo.colorAttachmentNames_.size()
            ),
            layout,
            subpass_name
        );
    }

    return GraphicsPass(renderPass, subpasses, attachments_, descriptorSetLayouts);
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

    attachments_[attachment_name].description = attachment;
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachmentSwapchain() {
    addAttachment(GraphicsPass::SwapchainName, device_.getSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR,
                  VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addSubpass(const std::string &subpass_name,
                                                     const SubpassInfo &subpass) {
    subpassInfos_[subpass_name] = subpass;
    subpassOrder_.emplace_back(subpass_name);
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addSubpassDependency(const std::string &src_subpass_name,
                                                               const std::string &dst_subpass_name,
                                                               VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                                               VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags) {
    DependencyInfo dependency = {};
    dependency.srcSubpass = src_subpass_name;
    dependency.dstSubpass = dst_subpass_name;
    dependency.srcStageMask = src_stage_mask;
    dependency.dstStageMask = dst_stage_mask;
    dependency.srcAccessFlags = src_access_flags;
    dependency.dstAccessFlags = dst_access_flags;

    subpassDependencyInfo_.emplace_back(dependency);
    return *this;
}

SubpassBuilder &SubpassBuilder::addShader(Shader::StageEnum shader_stage, const std::string &shader_path) {
    subpass_.shaders_.emplace_back(shader_stage, shader_path);
    return *this;
}

SubpassBuilder &SubpassBuilder::addVertexDescription(const std::vector<VkVertexInputBindingDescription> &bindings,
                                                     const std::vector<VkVertexInputAttributeDescription> &attributes) {
    subpass_.vertexDescription_ = VertexDescription(bindings, attributes);
    return *this;
}

SubpassBuilder &SubpassBuilder::addInputAttachment(const std::string &attachment_name, u32 set, u32 binding) {
    subpass_.inputAttachmentNames_.emplace_back(attachment_name);
    addDescriptor(set, binding, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    return *this;
}

SubpassBuilder &SubpassBuilder::addColorAttachment(const std::string &attachment_name) {
    subpass_.colorAttachmentNames_.emplace_back(attachment_name);
    return *this;
}

SubpassInfo SubpassBuilder::build() {
    return subpass_;
}

SubpassBuilder &SubpassBuilder::addDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags,
                                              VkDescriptorType type, u32 count) {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stage_flags;

    subpass_.descriptors_[set][binding] = layoutBinding;
    return *this;
}

}
