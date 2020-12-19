#ifndef IVY_RENDER_DEVICE_H
#define IVY_RENDER_DEVICE_H

#include <ivy/types.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <queue>
#include <functional>

namespace ivy {
class Engine;
class Platform;
struct Options;
}

namespace ivy::gfx {

class Shader {
public:
    enum class StageEnum {
        VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
        GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
        FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
        COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT
    };

    Shader(StageEnum stage, const std::string &path) : stage_(stage), path_(path) {}

    VkShaderStageFlagBits getStage() const {
        return static_cast<VkShaderStageFlagBits>(stage_);
    }

    std::string getShaderPath() const {
        return path_;
    }

private:
    StageEnum stage_;
    std::string path_;
};

class VertexDescription {
public:
    VertexDescription(VkVertexInputBindingDescription binding,
                      const std::vector<VkVertexInputAttributeDescription> &attributes) :
        binding_(binding), attributes_(attributes) {}

    const VkVertexInputBindingDescription &getBinding() const {
        return binding_;
    }

    const std::vector<VkVertexInputAttributeDescription> &getAttributes() const {
        return attributes_;
    }

private:
    VkVertexInputBindingDescription binding_;
    std::vector<VkVertexInputAttributeDescription> attributes_;
};

/**
 * \brief Abstraction around logical device for rendering
 */
class RenderDevice final {
public:
    explicit RenderDevice(const Options &options, const Platform &platform);
    ~RenderDevice();

    /**
     * \brief Get format of swapchain image views
     * \return Swapchain format
     */
    VkFormat getSwapchainFormat() const {
        return swapchainFormat_;
    }

    /**
     * \brief Create a render pass
     * \param attachments A vector of attachments
     * \param subpasses A vector of subpasses that use attachments
     * \param dependencies A vector describing dependencies between subpasses
     * \return VkRenderPass
     */
    VkRenderPass createRenderPass(const std::vector<VkAttachmentDescription> &attachments,
                                  const std::vector<VkSubpassDescription> &subpasses,
                                  const std::vector<VkSubpassDependency> &dependencies);

    /**
     * \brief Create a pipeline layout, TODO: allow descriptor sets and push constants to be defined
     * \return VkPipelineLayout
     */
    VkPipelineLayout createLayout();

    /**
     * \brief Create a graphics pipeline
     * \param shaders A vector of shaders
     * \param vertex_description Vertex description for shaders
     * \param layout Pipeline layout
     * \param render_pass Render pass
     * \return VkPipeline
     */
    VkPipeline createGraphicsPipeline(const std::vector<Shader> &shaders, const VertexDescription &vertex_description,
                                      VkPipelineLayout layout, VkRenderPass render_pass);

    // TODO: presentation

private:
    /**
     * \brief Choose a physical device
     */
    void choosePhysicalDevice();

    /**
     * \brief Create the swapchain and images for swapchain
     */
    void createSwapchain();

    const Options &options_;

    std::queue<std::function<void()>> cleanupQueue_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    u32 graphicsFamilyIndex_ = 0;
    u32 computeFamilyIndex_ = 0;
    u32 presentFamilyIndex_ = 0;

    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_;
    VkQueue computeQueue_;
    VkQueue presentQueue_;

    VkSwapchainKHR swapchain_;
    VkExtent2D swapchainExtent_;
    VkFormat swapchainFormat_;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
};

}

#endif // IVY_RENDER_DEVICE_H
