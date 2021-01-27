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

// TODO: push constants

// <set, <binding, VkDescriptorSetLayoutBinding>>
using LayoutBindingsMap_t = std::unordered_map<u32, std::unordered_map<u32, VkDescriptorSetLayoutBinding>>;

/**
 * \brief Holds pipeline and set layouts for a subpass
 */
struct SubpassLayout {
    VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> setLayouts;
};

/**
 * \brief Represents a subpass in a graphics pass
 */
class Subpass {
public:
    Subpass(VkPipeline pipeline, const SubpassLayout &layout, const std::string &name)
        : pipeline_(pipeline), layout_(layout), name_(name) {}

    /**
     * \brief Get the pipeline
     * \return VkPipeline
     */
    [[nodiscard]] VkPipeline getPipeline() const {
        return pipeline_;
    }

    /**
     * \brief Get the pipeline layout
     * \return VkPipelineLayout
     */
    [[nodiscard]] VkPipelineLayout getPipelineLayout() const {
        return layout_.pipelineLayout;
    }

    /**
     * \brief Get the descriptor set layout for a given set
     * \param set_index Which set's layout should be gotten
     * \return VkDescriptorSetLayout
     */
    [[nodiscard]] VkDescriptorSetLayout getSetLayout(u32 set_index) {
        return layout_.setLayouts.at(set_index);
    }

    /**
     * \brief Get the name of the subpass
     * \return Subpass name
     */
    [[nodiscard]] const std::string &getName() const {
        return name_;
    }

private:
    VkPipeline pipeline_;
    SubpassLayout layout_;
    std::string name_;
};

/**
 * \brief Stores data for an attachment
 */
struct AttachmentInfo {
    VkAttachmentDescription description;
    VkImageUsageFlags usage;
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

  /**
   * \brief Get the VkRenderPass for this graphics pass
   * \return VkRenderPass
   */
    [[nodiscard]] VkRenderPass getVkRenderPass() const {
        return renderPass_;
    }

    /**
     * \brief Get a subpass in this graphics pass
     * \param subpass_index The index of the subpass to get
     * \return Subpass
     */
    [[nodiscard]] Subpass getSubpass(u32 subpass_index) const {
        return subpasses_.at(subpass_index);
    }

    /**
     * \brief Get the attachment infos for the graphics pass
     * \return An unordered map with attachment names as the key and AttachmentInfo as the value
     */
    [[nodiscard]] const std::unordered_map<std::string, AttachmentInfo> &getAttachmentInfos() const {
        return attachmentInfos_;
    }

    /**
     * \brief Get the descriptor set layout for a given subpass and set
     * \param subpass_index The subpass
     * \param set_index The set in the subpass
     * \return DescriptorSetLayout
     */
    [[nodiscard]] const DescriptorSetLayout &getDescriptorSetLayout(u32 subpass_index, u32 set_index) const {
        return descriptorSetLayouts_.at(subpass_index).at(set_index);
    }

private:
    VkRenderPass renderPass_;
    std::vector<Subpass> subpasses_;
    std::unordered_map<std::string, AttachmentInfo> attachmentInfos_;
    std::unordered_map<u32, std::unordered_map<u32, DescriptorSetLayout>> descriptorSetLayouts_;
};

/**
 * \brief Subpass info used for building a graphics pass
 */
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
 * \brief Used to build a subpass description for a graphics pass builder
 */
class SubpassBuilder {
public:
    /**
     * \brief Add a shader to the subpass
     * \param shader_stage What type of shader
     * \param shader_path Where the shader bytecode is located
     * \return SubpassBuilder
     */
    SubpassBuilder &addShader(Shader::StageEnum shader_stage, const std::string &shader_path);

    // TODO: support multiple vertex descriptions

    /**
     * \brief Add a vertex description to the subpass
     * \param bindings Vertex input binding descriptions
     * \param attributes Vertex input attribute descriptions
     * \return SubpassBuilder
     */
    SubpassBuilder &addVertexDescription(const std::vector<VkVertexInputBindingDescription> &bindings = {},
                                         const std::vector<VkVertexInputAttributeDescription> &attributes = {});

    /**
     * \brief Add an input attachment to the subpass
     * \param attachment_name Name of the attachment that should be an input
     * \param set Which descriptor set the input attachment should belong to
     * \param binding Which binding in the descriptor set the input attachment should belong to
     * \return SubpassBuilder
     */
    SubpassBuilder &addInputAttachment(const std::string &attachment_name, u32 set, u32 binding);

    /**
     * \brief Add a color attachment to the subpass
     * \param attachment_name Name of the attachment
     * \return SubpassBuilder
     */
    SubpassBuilder &addColorAttachment(const std::string &attachment_name);

    /**
     * \brief Add a descriptor to the subpass
     * \param set The descriptor set index
     * \param binding The binding in the descriptor set
     * \param stage_flags Which shader stage the descriptor set belongs to
     * \param type What type of descriptor
     * \param count How many of the descriptor to add
     * \return SubpassBuilder
     */
    SubpassBuilder &addDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags, VkDescriptorType type,
                                  u32 count = 1);

    /**
     * \brief Build the subpass
     * \return SubpassInfo
     */
    SubpassInfo build();

private:
    SubpassInfo subpass_;
};

/**
 * \brief Used to build a graphics pass
 */
class GraphicsPassBuilder {
public:
    explicit GraphicsPassBuilder(RenderDevice &device)
        : device_(device) {}

    /**
     * \brief Add an attachment to the graphics pass
     * \param attachment_name The name of the attachment
     * \param format The format of the attachment
     * \param load_op The load op for the attachment
     * \param store_op The store op for the attachment
     * \param stencil_load_op The stencil load op for the attachment
     * \param stencil_store_op The stencil store op for the attachment
     * \param initial_layout The initial layout for the attachment
     * \param final_layout The final layout for the attachment
     * \return GraphicsPassBuilder
     */
    GraphicsPassBuilder &addAttachment(const std::string &attachment_name, VkFormat format,
                                       VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE,
                                       VkAttachmentLoadOp stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                       VkAttachmentStoreOp stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                       VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                                       VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /**
     * \brief Add an attachment to reference the swapchain
     * \return GraphicsPassBuilder
     */
    GraphicsPassBuilder &addAttachmentSwapchain();

    /**
     * \brief Add a subpass to the graphics pass
     * \param subpass_name The name of the subpass
     * \param subpass The subpass info built by SubpassBuilder
     * \return GraphicsPassBuilder
     */
    GraphicsPassBuilder &addSubpass(const std::string &subpass_name, const SubpassInfo &subpass);

    /**
     * \brief Add a subpass dependency to the graphics pass
     * \param src_subpass_name The source subpass
     * \param dst_subpass_name The destination subpass that depends on the source subpass
     * \param src_stage_mask Which pipeline stage the attachment is used in the source subpass
     * \param dst_stage_mask Which pipeline stage the attachment is used in the destination subpass
     * \param src_access_flags How the attachment is accessed in the source subpass
     * \param dst_access_flags How the attachment is accessed in the destination subpass
     * \return GraphicsPassBuilder
     */
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
