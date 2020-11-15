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
    /**
     * \brief Get a vector of required instance extensions, will error if at least one extension is unsupported
     * \return Vector of supported instance extensions
     */
    std::vector<const char *> getInstanceExtensions();

    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
};

}

#endif // IVY_RENDER_CONTEXT_H
