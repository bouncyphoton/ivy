#ifndef IVY_GRAPHICS_PASS_H
#define IVY_GRAPHICS_PASS_H

#include "ivy/graphics/shader.h"
#include "ivy/graphics/vertex_description.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace ivy::gfx {

class RenderDevice;

// TODO: this file is a little ugly/hard to read. Names of structures could be improved/re-organized

struct AttachmentDescription {
    VkAttachmentDescription vkAttachmentDescription;
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

    explicit GraphicsPass(VkRenderPass render_pass, const std::vector<VkPipeline> &subpass_pipelines,
                          const std::unordered_map<std::string, AttachmentDescription> &attachment_descriptions)
        : renderPass_(render_pass), subpassPipelines_(subpass_pipelines),
          attachmentDescriptions_(attachment_descriptions) {}

    [[nodiscard]] VkRenderPass getVkRenderPass() const {
        return renderPass_;
    }

    [[nodiscard]] const std::vector<VkPipeline> &getPipelines() const {
        return subpassPipelines_;
    }

    [[nodiscard]] const std::unordered_map<std::string, AttachmentDescription> &getAttachmentDescriptions() const {
        return attachmentDescriptions_;
    }

private:
    VkRenderPass renderPass_;
    std::vector<VkPipeline> subpassPipelines_;
    std::unordered_map<std::string, AttachmentDescription> attachmentDescriptions_;
};

struct SubpassDescription {
private:
    friend class SubpassBuilder;
    friend class GraphicsPassBuilder;

    std::vector<Shader> shaders_;
    VertexDescription vertexDescription_;
    std::vector<std::string> inputAttachmentNames_;
    std::vector<std::string> colorAttachmentNames_;
};

/**
 * \brief Build a subpass description for a graphics pass
 */
class SubpassBuilder {
public:
    SubpassBuilder &addShader(Shader::StageEnum shader_stage, const std::string &shader_path);

    SubpassBuilder &addVertexDescription(const std::vector<VkVertexInputBindingDescription> &bindings = {},
                                         const std::vector<VkVertexInputAttributeDescription> &attributes = {});

    SubpassBuilder &addInputAttachment(const std::string &attachment_name);

    SubpassBuilder &addColorAttachment(const std::string &attachment_name);

    SubpassDescription build();

private:
    SubpassDescription subpass_;
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

    GraphicsPassBuilder &addSubpass(const std::string &subpass_name, const SubpassDescription &subpass);

    GraphicsPassBuilder &addSubpassDependency(const std::string &src_subpass_name, const std::string &dst_subpass_name,
                                              VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                              VkAccessFlags src_access_flags, VkAccessFlags dst_access_flags);

    GraphicsPass build();
private:
    struct SubpassDependency {
        std::string srcSubpass;
        std::string dstSubpass;
        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;
        VkAccessFlags srcAccessFlags;
        VkAccessFlags dstAccessFlags;
    };

    RenderDevice &device_;
    std::unordered_map<std::string, AttachmentDescription> attachments_;
    std::unordered_map<std::string, SubpassDescription> subpassDescriptions_;
    std::vector<std::string> subpassOrder_;
    std::vector<SubpassDependency> subpassDependencies_;
};

}

#endif // IVY_GRAPHICS_PASS_H
