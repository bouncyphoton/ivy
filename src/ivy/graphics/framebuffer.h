#ifndef IVY_FRAMEBUFFER_H
#define IVY_FRAMEBUFFER_H

#include "ivy/types.h"
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>

namespace ivy::gfx {

/**
 * \brief Wrapper around Vulkan framebuffer
 */
class Framebuffer {
public:
    Framebuffer(VkFramebuffer framebuffer, VkExtent2D extent, const std::unordered_map<std::string, VkImageView> &views)
        : framebuffer_(framebuffer), extent_(extent), views_(views) {}

    [[nodiscard]] VkFramebuffer getVkFramebuffer() const {
        return framebuffer_;
    }

    [[nodiscard]] VkExtent2D getExtent() const {
        return extent_;
    }

    [[nodiscard]] const std::unordered_map<std::string, VkImageView> &getViews() const {
        return views_;
    }

private:
    VkFramebuffer framebuffer_;
    VkExtent2D extent_;
    std::unordered_map<std::string, VkImageView> views_;
};

}

#endif // IVY_FRAMEBUFFER_H
