#ifndef IVY_RENDER_CONTEXT_H
#define IVY_RENDER_CONTEXT_H

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
    std::vector<const char *> getInstanceExtensions();

    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
};

}

#endif // IVY_RENDER_CONTEXT_H
