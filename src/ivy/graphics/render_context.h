#ifndef IVY_RENDER_CONTEXT_H
#define IVY_RENDER_CONTEXT_H

#include <ivy/types.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace ivy {
    class Engine;
    class Platform;
    struct Options;
}

namespace ivy::gfx {

/**
 * \brief Vulkan abstraction layer
 */
class RenderContext final {
public:
    explicit RenderContext(const Options &options, const Platform &platform);
    ~RenderContext();

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

#endif // IVY_RENDER_CONTEXT_H
