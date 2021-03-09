#ifndef IVY_GRAPHICS_PASS_H
#define IVY_GRAPHICS_PASS_H

#include "ivy/types.h"
#include "ivy/graphics/shader.h"
#include "ivy/graphics/vertex_description.h"
#include "ivy/graphics/descriptor_set.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <optional>

namespace ivy::gfx {

class RenderDevice;

// TODO: push constants

// <set, <binding, VkDescriptorSetLayoutBinding>>
using LayoutBindingsMap_t = std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>>;

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
                          const std::map<std::string, AttachmentInfo> &attachment_infos,
                          const std::map<u32, std::map<u32, DescriptorSetLayout>> &descriptorSetLayouts, VkExtent2D extent)
        : renderPass_(render_pass), subpasses_(subpasses), attachmentInfos_(attachment_infos),
          descriptorSetLayouts_(descriptorSetLayouts), passExtent_(extent) {}

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
     * \return A map with attachment names as the key and AttachmentInfo as the value
     */
    [[nodiscard]] const std::map<std::string, AttachmentInfo> &getAttachmentInfos() const {
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

    /**
     * \brief Get the width and height of the graphics pass
     * \return VkExtent2D
     */
    [[nodiscard]] VkExtent2D getExtent() const {
        return passExtent_;
    }

private:
    // Friend so that they can access UnusedName
    friend class GraphicsPassBuilder;
    friend class SubpassBuilder;

    /**
     * \brief Used for referencing unused attachments
     */
    inline static const char *UnusedName = "__unused";

    VkRenderPass renderPass_;
    std::vector<Subpass> subpasses_;
    std::map<std::string, AttachmentInfo> attachmentInfos_;
    std::map<u32, std::map<u32, DescriptorSetLayout>> descriptorSetLayouts_;
    VkExtent2D passExtent_;
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
    std::optional<std::string> depthAttachmentName_;

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
     * \param set Which descriptor set the descriptor should belong to
     * \param binding Which binding in the descriptor set the descriptor should belong to
     * \param attachment_name Name of the attachment that should be an input
     * \return SubpassBuilder
     */
    SubpassBuilder &addInputAttachmentDescriptor(u32 set, u32 binding, const std::string &attachment_name);

    /**
     * \brief Add a uniform buffer to the subpass
     * \param set Which descriptor set the descriptor should belong to
     * \param binding Which binding in the descriptor set the descriptor should belong to
     * \param stage_flags Which shader stage the descriptor set belongs to
     * \return SubpassBuilder
     */
    SubpassBuilder &addUniformBufferDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags);

    /**
     * \brief Add a texture to sample to the subpass
     * \param set Which descriptor set the descriptor should belong to
     * \param binding Which binding in the descriptor set the descriptor should belong to
     * \param stage_flags Which shader stage the descriptor set belongs to
     * \return SubpassBuilder
     */
    SubpassBuilder &addTextureDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags);

    /**
     * \brief Add a color attachment to the subpass
     * \param attachment_name Name of the attachment to reference
     * \param location The location of the color attachment
     * \return SubpassBuilder
     */
    SubpassBuilder &addColorAttachment(const std::string &attachment_name, u32 location);

    /**
     * \brief Add a depth attachment to the subpass
     * \param attachment_name Name of the attachment to reference
     * \return SubpassBuilder
     */
    SubpassBuilder &addDepthAttachment(const std::string &attachment_name);

    /**
     * \brief Build the subpass
     * \return SubpassInfo
     */
    SubpassInfo build();

private:
    /**
     * \brief Add a descriptor to the subpass
     * \param set The descriptor set index
     * \param binding The binding in the descriptor set
     * \param stage_flags Which shader stage the descriptor set belongs to
     * \param type What type of descriptor
     * \return SubpassBuilder
     */
    SubpassBuilder &addDescriptor(u32 set, u32 binding, VkShaderStageFlags stage_flags, VkDescriptorType type);

    SubpassInfo subpass_;
};

/**
 * \brief Used to build a graphics pass
 */
class GraphicsPassBuilder {
public:
    explicit GraphicsPassBuilder(RenderDevice &device);

    /**
     * \brief Add an attachment to the graphics pass. Load and store ops as well as final layout are deduced from format
     * \param attachment_name The name of the attachment
     * \param format The format of the attachment
     * \param additional_usage Additional usage flags to be OR'd with deduced usage flags
     * \return GraphicsPassBuilder
     */
    GraphicsPassBuilder &addAttachment(const std::string &attachment_name, VkFormat format,
                                       VkImageUsageFlags additional_usage = 0);

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
     * \param usage The usage flags for this attachment
     * \return GraphicsPassBuilder
     */
    GraphicsPassBuilder &addAttachment(const std::string &attachment_name, VkFormat format,
                                       VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op,
                                       VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op,
                                       VkImageLayout initial_layout, VkImageLayout final_layout,
                                       VkImageUsageFlags usage);

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

    /**
     * \brief Set the extent of the attachments in this graphics pass (if not set, this defaults to swapchain extent)
     * \return GraphicsPassBuilder
     */
    GraphicsPassBuilder &setExtent(u32 width, u32 height);

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
    std::map<std::string, AttachmentInfo> attachments_;
    std::unordered_map<std::string, SubpassInfo> subpassInfos_;
    std::vector<std::string> subpassOrder_;
    std::vector<DependencyInfo> subpassDependencyInfo_;
    VkExtent2D extent_;
};

}

#endif // IVY_GRAPHICS_PASS_H
