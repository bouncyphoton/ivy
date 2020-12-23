#ifndef IVY_GRAPHICS_PASS_H
#define IVY_GRAPHICS_PASS_H

#include "ivy/graphics/render_device.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <utility>

namespace ivy::gfx {

// TODO: this file is a little ugly/hard to read. Names could be improved

/**
 * \brief Wrapper around Vulkan render passes for graphics
 */
class GraphicsPass {
public:
    /**
     * \brief Used for referencing the swapchain in attachments and subpasses
     */
    inline static const char *SwapchainName = "__swapchain";

    explicit GraphicsPass(VkRenderPass render_pass, const std::vector<VkPipeline> &subpass_pipelines)
        : renderPass_(render_pass), subpassPipelines_(subpass_pipelines) {}

    [[nodiscard]] VkRenderPass getVkRenderPass() const {
        return renderPass_;
    }

    [[nodiscard]] const std::vector<VkPipeline> &getPipelines() const {
        return subpassPipelines_;
    }

private:
    VkRenderPass renderPass_;
    std::vector<VkPipeline> subpassPipelines_;
};

struct SubpassDescription {
private:
    friend class SubpassBuilder;
    friend class GraphicsPassBuilder;

    std::vector<Shader> shaders_;
    VertexDescription vertexDescription_;
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

    SubpassBuilder &addColorAttachment(const std::string &attachment_name);

    SubpassDescription build();

private:
    SubpassDescription subpass_;
};

class GraphicsPassBuilder {
public:
    explicit GraphicsPassBuilder(RenderDevice &device)
        : device_(device) {}

    GraphicsPassBuilder &addAttachment(const std::string &attachment_name, VkFormat format, VkAttachmentLoadOp load_op,
                                       VkAttachmentStoreOp store_op, VkAttachmentLoadOp stencil_load_op,
                                       VkAttachmentStoreOp stencil_store_op, VkImageLayout initial_layout, VkImageLayout final_layout);

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
    std::unordered_map<std::string, VkAttachmentDescription> attachments_;
    std::unordered_map<std::string, SubpassDescription> subpassDescriptions_;
    std::vector<std::string> subpassOrder_;
    std::vector<SubpassDependency> subpassDependencies_;
};

}

#endif // IVY_GRAPHICS_PASS_H
