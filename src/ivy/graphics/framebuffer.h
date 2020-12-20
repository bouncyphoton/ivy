#ifndef IVY_FRAMEBUFFER_H
#define IVY_FRAMEBUFFER_H

#include "ivy/types.h"
#include <vulkan/vulkan.h>

namespace ivy::gfx {

/**
 * \brief Wrapper around Vulkan framebuffer
 */
class Framebuffer {
public:

    [[nodiscard]] VkFramebuffer getVkFramebuffer() const {
        return framebuffer_;
    }

    [[nodiscard]] VkExtent2D getExtent() const {
        return extent_;
    }

private:
    friend class RenderDevice;

    Framebuffer(VkFramebuffer framebuffer, VkExtent2D extent)
        : framebuffer_(framebuffer), extent_(extent) {}

    VkFramebuffer framebuffer_;
    VkExtent2D extent_;
};

}

#endif // IVY_FRAMEBUFFER_H
