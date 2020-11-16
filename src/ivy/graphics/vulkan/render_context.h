#ifndef IVY_RENDER_CONTEXT_H
#define IVY_RENDER_CONTEXT_H

#include <ivy/types.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace ivy {

class Engine;
class Platform;

/**
 * \brief Vulkan abstraction layer
 */
class RenderContext final {
public:
    explicit RenderContext(const Platform &platform);
    ~RenderContext();

private:
    /**
     * \brief Choose a physical device
     */
    void choosePhysicalDevice();

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
};

}

#endif // IVY_RENDER_CONTEXT_H
