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
    Framebuffer(VkFramebuffer framebuffer, VkExtent2D extent, const std::unordered_map<std::string, VkImageView> &views,
                const std::unordered_map<std::string, VkImage> &images)
        : framebuffer_(framebuffer), extent_(extent), views_(views), images_(images) {}

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
     * \brief Get the image view for an attachment
     * \param attachment_name Name of the attachment
     * \return VkImageView
     */
    [[nodiscard]] VkImageView getView(const std::string &attachment_name) const {
        return views_.at(attachment_name);
    }

    /**
     * \brief Get the image for an attachment
     * \param attachment_name Name of the attachment
     * \return VkImage
     */
    [[nodiscard]] VkImage getImage(const std::string &attachment_name) const {
        return images_.at(attachment_name);
    }

private:
    VkFramebuffer framebuffer_;
    VkExtent2D extent_;
    std::unordered_map<std::string, VkImageView> views_;
    std::unordered_map<std::string, VkImage> images_;
};

}

#endif // IVY_FRAMEBUFFER_H
