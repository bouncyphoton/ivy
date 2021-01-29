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

    /**
     * \brief Get the underlying VkFramebuffer
     * \return VkFramebuffer
     */
    [[nodiscard]] VkFramebuffer getVkFramebuffer() const {
        return framebuffer_;
    }

    /**
     * \brief Get the extent of the framebuffer
     * \return VkExtent2D
     */
    [[nodiscard]] VkExtent2D getExtent() const {
        return extent_;
    }

    /**
     * \brief Get an unordered map of VkImageViews with attachment names as keys
     * \return unordered_map<(std::string) AttachmentName, VkImageVIew>
     */
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
