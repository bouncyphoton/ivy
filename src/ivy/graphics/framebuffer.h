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
    Framebuffer(VkFramebuffer framebuffer, VkExtent2D extent)
        : framebuffer_(framebuffer), extent_(extent) {}

    [[nodiscard]] VkFramebuffer getVkFramebuffer() const {
        return framebuffer_;
    }

    [[nodiscard]] VkExtent2D getExtent() const {
        return extent_;
    }

private:
    VkFramebuffer framebuffer_;
    VkExtent2D extent_;
};

}

#endif // IVY_FRAMEBUFFER_H
