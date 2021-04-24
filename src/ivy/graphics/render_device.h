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
     * \brief Get graphics queue
     * \return VkQueue
     */
    VkQueue getGraphicsQueue();

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
     * \brief Get the extent of thevswapchain
     * \return Swapchain extent
     */
    [[nodiscard]] VkExtent2D getSwapchainExtent() const {
        return swapchainExtent_;
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
     * \param num_color_attachments The number of color attachments for the pipeline
     * \param has_depth_attachment Whether or not there is a depth attachment
     * \param state State settings for graphics pipeline
     * \return VkPipeline
     */
    VkPipeline createGraphicsPipeline(const std::vector<Shader> &shaders, const VertexDescription &vertex_description,
                                      VkPipelineLayout layout, VkRenderPass render_pass, u32 subpass,
                                      u32 num_color_attachments, bool has_depth_attachment,
                                      const GraphicsPipelineState &state);

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
    VkBuffer createVertexBuffer(const void *data, VkDeviceSize size);

    /**
     * \brief Create an index buffer (of u32s) with the lifetime of the render device
     * \param data Pointer to index data
     * \param size Size of index buffer in bytes
     * \return VkBuffer
     */
    VkBuffer createIndexBuffer(const void *data, VkDeviceSize size);

    /**
     * \brief Create an image on the GPU using given image data with the lifetime of the render device
     * \param image_ci Image create info
     * \param image_view_ci Image view create info
     * \param data Pixel data
     * \param size Size of pixel data in bytes
     * \return VkImage and VkImageView pair
     */
    std::pair<VkImage, VkImageView> createTextureGPUFromData(VkImageCreateInfo image_ci,
                                                             VkImageViewCreateInfo image_view_ci,
                                                             const void *data, VkDeviceSize size);

    /**
     * \brief Create a sampler
     * \param mag_filter Magnification filter
     * \param min_filter Minification filter
     * \param u_wrap Wrapping for u addressing
     * \param v_wrap Wrapping for v addressing
     * \param w_wrap Wrapping for w addressing
     * \return VkSampler
     */
    VkSampler createSampler(VkFilter mag_filter, VkFilter min_filter,
                            VkSamplerAddressMode u_wrap = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                            VkSamplerAddressMode v_wrap = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                            VkSamplerAddressMode w_wrap = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    /**
     * \brief Get a VkDescriptorSet with data specified in set for a graphics pass for the current frame
     * \param pass The associated graphics pass
     * \param set The set
     * \return VkDescriptorSet ready for binding
     */
    VkDescriptorSet getVkDescriptorSet(const GraphicsPass &pass, const DescriptorSet &set);

    /**
     * \brief From a list, get the first format that is supported by the device with given features
     * \param formats List of formats to check
     * \param feature The feature flag bits to check
     * \param tiling The tiling
     * \return First valid format from list or VK_FORMAT_UNDEFINED if none are valid
     */
    VkFormat getFirstSupportedFormat(const std::vector<VkFormat> &formats, VkFormatFeatureFlags feature,
                                     VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);

private:
    /**
     * \brief Choose a physical device
     */
    void choosePhysicalDevice();

    /**
     * \brief Create the swapchain and images for swapchain
     */
    void createSwapchain();

    /**
     * \brief Create a buffer on the GPU with the lifetime of the render device
     * \param data Pointer to the data
     * \param size Size of the buffer in bytes
     * \param usage How the buffer will be used
     * \return VkBuffer
     */
    VkBuffer createBufferGPU(const void *data, VkDeviceSize size, VkBufferUsageFlagBits usage);

    /**
     * \brief Create a staging src buffer on the CPU, you need to destroy this yourself with vmaDestroyBuffer
     * \param data Pointer to the data
     * \param size Size of the buffer in bytes
     * \return VkBuffer
     */
    std::pair<VkBuffer, VmaAllocation> createStagingBufferCPU(const void *data, VkDeviceSize size);

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
    u32 maxSets_ = 4096;

    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceSize> uniformBufferOffsets_;
    std::vector<void *> uniformBufferMappedPointers_;
    VkDeviceSize uniformBufferSize_;
};

}

#endif // IVY_RENDER_DEVICE_H
