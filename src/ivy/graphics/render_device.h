#ifndef IVY_RENDER_DEVICE_H
#define IVY_RENDER_DEVICE_H

#include "ivy/types.h"
#include "ivy/options.h"
#include "ivy/graphics/command_buffer.h"
#include "ivy/graphics/shader.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <string>
#include <stack>
#include <functional>

namespace ivy {
class Engine;
class Platform;
struct Options;
}

namespace ivy::gfx {

class VertexDescription {
public:
    VertexDescription(const std::vector<VkVertexInputBindingDescription> &bindings = {},
                      const std::vector<VkVertexInputAttributeDescription> &attributes = {}) :
        bindings_(bindings), attributes_(attributes) {}

    [[nodiscard]] const std::vector<VkVertexInputBindingDescription> &getBindings() const {
        return bindings_;
    }

    [[nodiscard]] const std::vector<VkVertexInputAttributeDescription> &getAttributes() const {
        return attributes_;
    }

private:
    std::vector<VkVertexInputBindingDescription> bindings_;
    std::vector<VkVertexInputAttributeDescription> attributes_;
};

/**
 * \brief Wrapper around logical device for rendering
 */
class RenderDevice final {
public:
    explicit RenderDevice(const Options &options, const Platform &platform);
    ~RenderDevice();

    /**
     * \brief Prepare and begin a frame for rendering
     */
    void beginFrame();

    /**
     * \brief Finish frame and present
     */
    void endFrame();

    /**
     * \brief Get the command buffer for the current frame
     * \return Command buffer for current frame
     */
    CommandBuffer getCommandBuffer();

    /**
     * \brief Get and submit a command buffer for recording one time commands (like a buffer copy)
     * \param queue The queue the command buffer should be submitted to
     * \param record_func A function that records commands into a command buffer
     */
    void submitOneTimeCommands(VkQueue queue, const std::function<void(CommandBuffer)> &record_func);

    /**
     * \brief Get format of swapchain image views
     * \return Swapchain format
     */
    [[nodiscard]] VkFormat getSwapchainFormat() const {
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
     * \param subpass Subpass index
     * \return VkPipeline
     */
    VkPipeline createGraphicsPipeline(const std::vector<Shader> &shaders, const VertexDescription &vertex_description,
                                      VkPipelineLayout layout, VkRenderPass render_pass, u32 subpass = 0);

    /**
     * \brief Get (or create if doesn't exist) the current swapchain framebuffer for a given render pass
     * \param render_pass The framebuffer's render pass
     * \return Current framebuffer
     */
    Framebuffer getSwapchainFramebuffer(VkRenderPass render_pass);

    /**
     * \brief Create a vertex buffer with the lifetime of the render device
     * \param data Pointer to vertex data
     * \param size Size of vertex data in bytes
     * \return VkBuffer
     */
    VkBuffer createVertexBuffer(void *data, VkDeviceSize size);

private:
    /**
     * \brief Choose a physical device
     */
    void choosePhysicalDevice();

    /**
     * \brief Create the swapchain and images for swapchain
     */
    void createSwapchain();

    const Options options_;

    std::stack<std::function<void()>> cleanupStack_;

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

    VmaAllocator allocator_;

    VkSwapchainKHR swapchain_;
    VkExtent2D swapchainExtent_;
    VkFormat swapchainFormat_;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::unordered_map<VkRenderPass, std::vector<Framebuffer>> swapchainFramebuffers_;

    VkCommandPool commandPool_;
    std::vector<VkCommandBuffer> commandBuffers_;

    u32 currentFrame_ = 0;
    u32 swapImageIndex_ = 0;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
};

}

#endif // IVY_RENDER_DEVICE_H
