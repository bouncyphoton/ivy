#ifndef IVY_GRAPHICS_PASS_H
#define IVY_GRAPHICS_PASS_H

#include "ivy/types.h"
#include "ivy/graphics/shader.h"
#include "ivy/graphics/vertex_description.h"
#include "ivy/graphics/descriptor_set.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace ivy::gfx {

class RenderDevice;

// TODO: this file is a little ugly/hard to read. Names of structures could be improved/re-organized
// TODO: push constants

// <set, <binding, VkDescriptorSetLayoutBinding>>
using LayoutBindingsMap_t = std::unordered_map<u32, std::unordered_map<u32, VkDescriptorSetLayoutBinding>>;

struct AttachmentInfo {
    VkAttachmentDescription description;
    VkImageUsageFlags usage;
};

// TODO: should this go somewhere else?
struct SubpassLayout {
    VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> setLayouts;
};

// TODO: is it confusing in build() ?
class Subpass {
public:
    Subpass(VkPipeline pipeline, const SubpassLayout &layout, const std::string &name)
        : pipeline_(pipeline), layout_(layout), name_(name) {}

    [[nodiscard]] VkPipeline getPipeline() const {
        return pipeline_;
    }

    [[nodiscard]] VkPipelineLayout getPipelineLayout() const {
        return layout_.pipelineLayout;
    }

    [[nodiscard]] VkDescriptorSetLayout getSetLayout(u32 set_index) {
        return layout_.setLayouts.at(set_index);
    }

    [[nodiscard]] const std::string &getName() const {
        return name_;
    }

private:
    VkPipeline pipeline_;
    SubpassLayout layout_;
    std::string name_;
};

/**
 * \brief Wrapper around Vulkan render passes for graphics
 */
class GraphicsPass {
public:
    /**
     * \brief Used for referencing the swapchain in attachments and subpasses
     */
    inline static const char *SwapchainName = "__swapchain";

    explicit GraphicsPass(VkRenderPass render_pass, const std::vector<Subpass> &subpasses,
                          const std::unordered_map<std::string, AttachmentInfo> &attachment_infos,
                          const std::unordered_map<u32, std::unordered_map<u32, DescriptorSetLayout>> &descriptorSetLayouts)
        : renderPass_(render_pass), subpasses_(subpasses),
          attachmentInfos_(attachment_infos), descriptorSetLayouts_(descriptorSetLayouts) {}

    [[nodiscard]] VkRenderPass getVkRenderPass() const {
        return renderPass_;
    }

    [[nodiscard]] Subpass getSubpass(u32 subpass_index) const {
        return subpasses_.at(subpass_index);
    }

    [[nodiscard]] const std::unordered_map<std::string, AttachmentInfo> &getAttachmentInfos() const {
        return attachmentInfos_;
    }

    [[nodiscard]] const DescriptorSetLayout &getDescriptorSetLayout(u32 subpass_index, u32 set_index) const {
        return descriptorSetLayouts_.at(subpass_index).at(set_index);
    }

private:
    VkRenderPass renderPass_;
    std::vector<Subpass> subpasses_;
    std::unordered_map<std::string, AttachmentInfo> attachmentInfos_;
    std::unordered_map<u32, std::unordered_map<u32, DescriptorSetLayout>> descriptorSetLayouts_;
};

struct SubpassInfo {
private:
    friend class SubpassBuilder;
    friend class GraphicsPassBuilder;

    std::vector<Shader> shaders_;
    VertexDescription vertexDescription_;
    std::vector<std::string> inputAttachmentNames_;
    std::vector<std::string> colorAttachmentNames_;

    LayoutBindingsMap_t descriptors_;
};

/**
 * \brief Build a subpass description for a graphics pass
 */
class SubpassBuilder {
public:
    SubpassBuilder &addShader(Shader::StageEnum shader_stage, const std::string &shader_path);

    SubpassBuilder &addVertexDescription(const std::vector<VkVertexInputBindingDescription> &bindings = {},
                                         const std::vector<VkVertexInputAttributeDescription> &attributes = {});

    SubpassBuilder &addInputAttachment(const std::string &attachment_name, u32 set, u32 binding);

    SubpassBuilder &addColorAttachment(const std::string &attachment_name);

    SubpassBuilder &addDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags, VkDescriptorType type,
                                  u32 count = 1);

    SubpassInfo build();

private:
    SubpassInfo subpass_;
};

class GraphicsPassBuilder {
public:
    explicit GraphicsPassBuilder(RenderDevice &device)
        : device_(device) {}

    GraphicsPassBuilder &addAttachment(const std::string &attachment_name, VkFormat format,
                                       VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE,
                                       VkAttachmentLoadOp stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                       VkAttachmentStoreOp stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                       VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                                       VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    GraphicsPassBuilder &addAttachmentSwapchain();

    GraphicsPassBuilder &addSubpass(const std::string &subpass_name, const SubpassInfo &subpass);

    GraphicsPassBuilder &addSubpassDependency(const std::string &src_subpass_name, const std::string &dst_subpass_name,
                                              VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                              VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags);

    GraphicsPass build();
private:
    struct DependencyInfo {
        std::string srcSubpass;
        std::string dstSubpass;
        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;
        VkAccessFlags srcAccessFlags;
        VkAccessFlags dstAccessFlags;
    };

    RenderDevice &device_;
    std::unordered_map<std::string, AttachmentInfo> attachments_;
    std::unordered_map<std::string, SubpassInfo> subpassInfos_;
    std::vector<std::string> subpassOrder_;
    std::vector<DependencyInfo> subpassDependencyInfo_;
};

}

#endif // IVY_GRAPHICS_PASS_H
