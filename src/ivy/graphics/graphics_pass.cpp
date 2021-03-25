#include "graphics_pass.h"
#include "ivy/graphics/render_device.h"
#include "ivy/graphics/vk_utils.h"
#include "ivy/graphics/texture.h"
#include "ivy/log.h"
#include "ivy/consts.h"
#include <unordered_set>

namespace ivy::gfx {

// TODO: would be cool to check shader bytecode to see if everything was referenced correctly

GraphicsPassBuilder::GraphicsPassBuilder(RenderDevice &device)
    : device_(device), extent_(device.getSwapchainExtent()) {}

GraphicsPass GraphicsPassBuilder::build() {
    Log::debug("Building graphics pass");

    //--------------------------------------
    // Prepare attachments for referencing
    //--------------------------------------

    // Holds VkAttachmentDescriptions for render pass creation
    std::vector<VkAttachmentDescription> attachmentDescriptions;

    // Used to turn an attachment name into an index for attachmentDescriptions
    std::unordered_map<std::string, u32> attachmentLocations;

    // A set of unreferenced attachments for validation
    std::unordered_set<std::string> unreferencedAttachments;

    for (const auto &attachmentPair : attachments_) {
        const std::string &name = attachmentPair.first;
        const AttachmentInfo &info = attachmentPair.second;
        Log::debug("- Processing attachment: %", name);

        attachmentLocations[name] = attachmentDescriptions.size();
        attachmentDescriptions.emplace_back(info.description);
        unreferencedAttachments.emplace(name);
    }

    //--------------------------------------
    // Process subpass data
    //--------------------------------------

    // Holds VkSubpassDescriptions for render pass creation
    std::vector<VkSubpassDescription> subpassDescriptions;

    // Turns a subpass name into an index into subpassDescriptions
    std::unordered_map<std::string, u32> subpassLocations;
    subpassLocations[GraphicsPass::SwapchainName] = VK_SUBPASS_EXTERNAL;

    // Holds attachment references for color attachments in subpasses by name <subpass_name, reference>
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> colorAttachmentReferences;

    // Holds attachment references for depth attachments in subpasses by name <subpass_name, reference>
    std::unordered_map<std::string, std::optional<VkAttachmentReference>> depthAttachmentReferences;

    // Holds attachment references for input attachments in subpasses by name <subpass_name, reference>
    std::unordered_map<std::string, std::vector<VkAttachmentReference>> inputAttachmentReferences;

    for (const std::string &subpassName : subpassOrder_) {
        Log::debug("- Processing subpass: %", subpassName);

        // Keep track of where this subpass is in subpassDescriptions vector
        subpassLocations[subpassName] = subpassDescriptions.size();

        // Process color attachments
        for (const std::string &colorAttachmentName : subpassInfos_[subpassName].colorAttachmentNames_) {
            unreferencedAttachments.erase(colorAttachmentName);

            // Mark unused attachments as VK_ATTACHMENT_UNUSED and continue
            if (colorAttachmentName == GraphicsPass::UnusedName) {
                VkAttachmentReference reference = {};
                reference.attachment = VK_ATTACHMENT_UNUSED;

                colorAttachmentReferences[subpassName].emplace_back(reference);
                continue;
            }

            Log::debug("  - Processing color attachment: %", colorAttachmentName);

            if constexpr (consts::DEBUG) {
                if (attachmentLocations.find(colorAttachmentName) == attachmentLocations.end()) {
                    Log::fatal("Color attachment '%' was not added to the graphics pass", colorAttachmentName);
                }
            }

            // Create an attachment reference for this color attachment
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[colorAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Add it to the list of color attachments for this subpass
            colorAttachmentReferences[subpassName].emplace_back(reference);
        }

        // Process depth attachment
        if (subpassInfos_[subpassName].depthAttachmentName_.has_value()) {
            const auto &depthAttachmentName = subpassInfos_[subpassName].depthAttachmentName_.value();
            unreferencedAttachments.erase(depthAttachmentName);

            Log::debug("  - Processing depth attachment: %", depthAttachmentName);

            if constexpr (consts::DEBUG) {
                if (attachmentLocations.find(depthAttachmentName) == attachmentLocations.end()) {
                    Log::fatal("Depth attachment '%' was not added to the graphics pass", depthAttachmentName);
                }
            }

            // Create an attachment reference for this depth attachment
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[depthAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Add it to the list of depth attachments for this subpass
            depthAttachmentReferences[subpassName] = reference;
        }

        // Process input attachments
        for (const std::string &inputAttachmentName : subpassInfos_[subpassName].inputAttachmentNames_) {
            unreferencedAttachments.erase(inputAttachmentName);

            Log::debug("  - Processing input attachment: %", inputAttachmentName);

            if constexpr (consts::DEBUG) {
                if (attachmentLocations.find(inputAttachmentName) == attachmentLocations.end()) {
                    Log::fatal("Input attachment '%' was not added to the graphics pass", inputAttachmentName);
                }
            }

            // Create an attachment reference for this input attachment
            VkAttachmentReference reference = {};
            reference.attachment = attachmentLocations[inputAttachmentName];
            reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Set the usage for this attachment
            attachments_[inputAttachmentName].usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

            // Add it to the list of input attachments for this subpass
            inputAttachmentReferences[subpassName].emplace_back(reference);
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
        if (depthAttachmentReferences[subpassName].has_value()) {
            subpassDescription.pDepthStencilAttachment = &depthAttachmentReferences[subpassName].value();
        }
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;

        subpassDescriptions.emplace_back(subpassDescription);
    }

    if constexpr (consts::DEBUG) {
        if (!unreferencedAttachments.empty()) {
            std::string errMsg = "% attachment% not referenced by any subpasses:";

            for (const auto &unreferenced : unreferencedAttachments) {
                errMsg += "\n- " + unreferenced;
            }

            Log::fatal(errMsg, unreferencedAttachments.size(),
                       unreferencedAttachments.size() == 1 ? " was" : "s were");
        }
    }

    //--------------------------------------
    // Process subpass dependencies
    //--------------------------------------

    Log::debug("- Building subpass dependencies");

    // Create dependencies
    std::vector<VkSubpassDependency> dependencies = {};
    for (const DependencyInfo &info : subpassDependencyInfo_) {
        Log::debug("  - Processing dependency: % -> %", info.srcSubpass, info.dstSubpass);

        if constexpr (consts::DEBUG) {
            if (subpassLocations.find(info.srcSubpass) == subpassLocations.end()) {
                Log::fatal("Subpass dependency could not be processed because '%' is not a subpass in this graphics pass",
                           info.srcSubpass);
            }

            if (subpassLocations.find(info.dstSubpass) == subpassLocations.end()) {
                Log::fatal("Subpass dependency could not be processed because '%' is not a subpass in this graphics pass",
                           info.dstSubpass);
            }
        }

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = subpassLocations.at(info.srcSubpass);
        dependency.dstSubpass = subpassLocations.at(info.dstSubpass);
        dependency.srcStageMask = info.srcStageMask;
        dependency.dstStageMask = info.dstStageMask;
        dependency.srcAccessMask = info.srcAccessFlags;
        dependency.dstAccessMask = info.dstAccessFlags;

        dependencies.emplace_back(dependency);
    }

    //--------------------------------------
    // Render pass creation
    //--------------------------------------

    VkRenderPass renderPass = device_.createRenderPass(attachmentDescriptions, subpassDescriptions, dependencies);

    //--------------------------------------
    // Build pipelines for subpasses
    //--------------------------------------

    Log::debug("- Building pipelines");

    // A vector of subpasses, this gets passed to the final GraphicsPass
    std::vector<Subpass> subpasses;

    // A table of DescriptorSetLayouts for the final GraphicsPass, <subpass, set, layout>
    std::map<u32, std::map<u32, DescriptorSetLayout>> descriptorSetLayouts;

    // Go over each subpass
    for (u32 subpassIdx = 0; subpassIdx < subpassOrder_.size(); ++subpassIdx) {
        const std::string &subpass_name = subpassOrder_[subpassIdx];
        const SubpassInfo &subpassInfo = subpassInfos_[subpass_name];

        Log::debug("  - Subpass % has % set%", subpass_name, subpassInfo.descriptors_.size(),
                   subpassInfo.descriptors_.size() != 1 ? "s" : "");

        // Create DescriptorSetLayouts for GraphicsPass
        for (const auto &descriptorSet : subpassInfo.descriptors_) {
            u32 setIdx = descriptorSet.first;
            const std::map<u32, VkDescriptorSetLayoutBinding> &bindings = descriptorSet.second;

            // Turn bindings map into a vector
            std::vector<VkDescriptorSetLayoutBinding> bindingsVector;
            bindingsVector.reserve(bindings.size());
            for (const auto &binding : bindings) {
                bindingsVector.emplace_back(binding.second);
            }

            // Store descriptor set layout
            descriptorSetLayouts[subpassIdx].emplace(setIdx, DescriptorSetLayout(subpassIdx, setIdx, bindingsVector));

            // Debug logging
            Log::debug("    - set % has % bindings", setIdx, bindings.size());
            for (u32 bindingIdx = 0; bindingIdx < bindingsVector.size(); ++bindingIdx) {
                Log::debug("      - binding %: %", bindingIdx,
                           vk_descriptor_type_to_string(bindingsVector[bindingIdx].descriptorType));
            }
        }

        // Create layout
        SubpassLayout layout = device_.createLayout(subpassInfo.descriptors_);

        // Create pipeline
        subpasses.emplace_back(
            device_.createGraphicsPipeline(
                subpassInfo.shaders_, subpassInfo.vertexDescription_, layout.pipelineLayout, renderPass,
                subpasses.size(), subpassInfo.colorAttachmentNames_.size(), subpassInfo.depthAttachmentName_.has_value(),
                subpassInfo.pipelineState_
            ),
            layout,
            subpass_name
        );
    }

    // Create the graphics pass
    return GraphicsPass(renderPass, subpasses, attachments_, descriptorSetLayouts, extent_);
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachment(const std::string &attachment_name,
                                                        const gfx::Texture &texture) {
    addAttachment(attachment_name, texture.getFormat());
    attachments_[attachment_name].texture = texture;
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachment(const std::string &attachment_name, VkFormat format,
                                                        VkImageUsageFlags additional_usage) {
    bool separateDepthStencilLayoutsEnabled = false;
    bool isDepth = format >= VK_FORMAT_D16_UNORM && format <= VK_FORMAT_D32_SFLOAT_S8_UINT && format != VK_FORMAT_S8_UINT;
    bool isStencil = format >= VK_FORMAT_S8_UINT && format <= VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp stencil_load_op = isStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp stencil_store_op = isStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout final_layout;
    VkImageUsageFlags usage = 0;
    if ((isDepth && isStencil) || (isDepth && !separateDepthStencilLayoutsEnabled)) {
        final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } else if (isDepth && separateDepthStencilLayoutsEnabled) {
        final_layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } else if (isStencil) {
        final_layout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } else {
        final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    return addAttachment(attachment_name, format, load_op, store_op, stencil_load_op, stencil_store_op,
                         initial_layout, final_layout, usage | additional_usage);
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachment(const std::string &attachment_name, VkFormat format,
                                                        VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op,
                                                        VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op,
                                                        VkImageLayout initial_layout, VkImageLayout final_layout,
                                                        VkImageUsageFlags usage) {
    if constexpr (consts::DEBUG) {
        if (attachments_.find(attachment_name) != attachments_.end()) {
            Log::fatal("Attachment '%' was added multiple times to the same graphics pass!", attachment_name);
        }
        if (attachment_name == GraphicsPass::UnusedName) {
            Log::fatal("'%' is a reserved attachment name", attachment_name);
        }
    }

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
    attachments_[attachment_name].usage = usage;
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addAttachmentSwapchain() {
    addAttachment(GraphicsPass::SwapchainName, device_.getSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR,
                  VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    return *this;
}

GraphicsPassBuilder &GraphicsPassBuilder::addSubpass(const std::string &subpass_name, const SubpassInfo &subpass) {
    if constexpr (consts::DEBUG) {
        if (subpass_name == GraphicsPass::SwapchainName) {
            Log::fatal("'%' is a reserved subpass name", subpass_name);
        }
        if (subpassInfos_.find(subpass_name) != subpassInfos_.end()) {
            Log::fatal("Subpass '%' was added multiple times to the same graphics pass!", subpass_name);
        }
    }

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

GraphicsPassBuilder &GraphicsPassBuilder::setExtent(u32 width, u32 height) {
    extent_.width = width;
    extent_.height = height;
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

SubpassBuilder &SubpassBuilder::addInputAttachmentDescriptor(u32 set, u32 binding, const std::string &attachment_name) {
    subpass_.inputAttachmentNames_.emplace_back(attachment_name);
    addDescriptor(set, binding, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    return *this;
}

SubpassBuilder &SubpassBuilder::addUniformBufferDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags) {
    addDescriptor(set, binding, stage_flags, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    return *this;
}

SubpassBuilder &SubpassBuilder::addTextureDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags) {
    addDescriptor(set, binding, stage_flags, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    return *this;
}

SubpassBuilder &SubpassBuilder::addColorAttachment(const std::string &attachment_name, u32 location) {
    std::vector<std::string> &names = subpass_.colorAttachmentNames_;

    // If we need to make the vector bigger before we can add our attachment, fill it with Unused
    if (location >= names.size()) {
        names.resize(location + 1, GraphicsPass::UnusedName);
    }

    // Set the attachment
    names[location] = attachment_name;

    return *this;
}

SubpassBuilder &SubpassBuilder::addDepthAttachment(const std::string &attachment_name) {
    if constexpr (consts::DEBUG) {
        if (subpass_.depthAttachmentName_.has_value()) {
            Log::fatal("Cannot add depth attachment '%' to subpass because '%' was already added",
                       attachment_name, subpass_.depthAttachmentName_.value());
        }
    }

    subpass_.depthAttachmentName_ = attachment_name;

    return *this;
}

SubpassBuilder &SubpassBuilder::setDepthTesting(bool depth_testing) {
    subpass_.pipelineState_.depthTestEnable = depth_testing;
    return *this;
}

SubpassBuilder &SubpassBuilder::setDepthWriting(bool depth_writing) {
    subpass_.pipelineState_.depthWriteEnable = depth_writing;
    return *this;
}

SubpassBuilder &SubpassBuilder::setDepthCompareOp(VkCompareOp compare_op) {
    subpass_.pipelineState_.depthCompareOp = compare_op;
    return *this;
}

SubpassBuilder &SubpassBuilder::setColorBlending(VkBlendOp blend_op, VkBlendFactor src_blend_factor,
                                                 VkBlendFactor dst_blend_factor) {
    subpass_.pipelineState_.colorBlendOp = blend_op;
    subpass_.pipelineState_.srcColorBlendFactor = src_blend_factor;
    subpass_.pipelineState_.dstColorBlendFactor = dst_blend_factor;
    return *this;
}

SubpassBuilder &SubpassBuilder::setAlphaBlending(VkBlendOp blend_op, VkBlendFactor src_blend_factor,
                                                 VkBlendFactor dst_blend_factor) {
    subpass_.pipelineState_.alphaBlendOp = blend_op;
    subpass_.pipelineState_.srcAlphaBlendFactor = src_blend_factor;
    subpass_.pipelineState_.dstAlphaBlendFactor = dst_blend_factor;
    return *this;
}

SubpassInfo SubpassBuilder::build() {
    return subpass_;
}

SubpassBuilder &SubpassBuilder::addDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags,
                                              VkDescriptorType type) {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stage_flags;

    subpass_.descriptors_[set][binding] = layoutBinding;
    return *this;
}

}
