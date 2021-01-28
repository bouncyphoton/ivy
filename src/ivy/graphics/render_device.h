#ifndef IVY_RENDER_DEVICE_H
#define IVY_RENDER_DEVICE_H

#include "ivy/types.h"
#include "ivy/options.h"
#include "ivy/graphics/command_buffer.h"
#include "ivy/graphics/shader.h"
#include "ivy/graphics/vertex_description.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/descriptor_set_cache.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <string>
#include <stack>
#include <functional>
#include <unordered_map>

namespace ivy {
class Engine;
class Platform;
struct Options;
}

namespace ivy::gfx {

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
     * \brief Create a layout for a subpass
     * \param layout_bindings An unordered map of unordered maps of VkDescriptorSetLayoutBinding.
     * The keys are set and binding respectively.
     * \return SubpassLayout
     */
    SubpassLayout createLayout(const LayoutBindingsMap_t &layout_bindings = {});

    /**
     * \brief Create a graphics pipeline
     * \param shaders A vector of shaders
     * \param vertex_description Vertex description for shaders
     * \param layout Pipeline layout
     * \param render_pass Render pass
     * \param subpass Subpass index
     * \param num_color_attachments The number of color attachments for the subpass
     * \return VkPipeline
     */
    VkPipeline createGraphicsPipeline(const std::vector<Shader> &shaders, const VertexDescription &vertex_description,
                                      VkPipelineLayout layout, VkRenderPass render_pass, u32 subpass, u32 num_color_attachments);

    /**
     * \brief Get (or create if doesn't exist) the current swapchain framebuffer for a given graphics pass
     * \param pass The graphics pass for the framebuffer to get
     * \return Framebuffer for graphics pass (for this frame)
     */
    Framebuffer &getFramebuffer(const GraphicsPass &pass);

    /**
     * \brief Create a vertex buffer with the lifetime of the render device
     * \param data Pointer to vertex data
     * \param size Size of vertex data in bytes
     * \return VkBuffer
     */
    VkBuffer createVertexBuffer(void *data, VkDeviceSize size);

    /**
     * \brief Get a VkDescriptorSet with data specified in set for a graphics pass for the current frame
     * \param pass The associated graphics pass
     * \param set The set
     * \return VkDescriptorSet ready for binding
     */
    VkDescriptorSet getVkDescriptorSet(const GraphicsPass &pass, const DescriptorSet &set);

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
    VkPhysicalDeviceLimits limits_ = {};
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
    std::unordered_map<VkRenderPass, std::vector<Framebuffer>> framebuffers_;

    VkCommandPool commandPool_;
    std::vector<VkCommandBuffer> commandBuffers_;

    u32 currentFrame_ = 0;
    u32 swapImageIndex_ = 0;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;

    std::vector<VkDescriptorPool> pools_;
    std::vector<DescriptorSetCache> descriptorSetCaches_;

    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceSize> uniformBufferOffsets_;
    std::vector<void *> uniformBufferMappedPointers_;
    VkDeviceSize uniformBufferSize_;
};

}

#endif // IVY_RENDER_DEVICE_H
