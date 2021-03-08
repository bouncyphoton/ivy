#ifndef IVY_TEXTURE2D_H
#define IVY_TEXTURE2D_H

#include "ivy/types.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace ivy::gfx {

class RenderDevice;

/**
 * \brief Holds image data for rendering
 */
class Texture2D {
public:
    Texture2D(RenderDevice &device, u32 width, u32 height, VkFormat format, const u8 *data, u32 size);

    [[nodiscard]] VkImageView getImageView() const {
        return imageView_;
    }

    [[nodiscard]] VkSampler getSampler() const {
        return sampler_;
    }

private:
    VkImage image_;
    VkImageView imageView_;
    VkSampler sampler_;
};

}

#endif // IVY_TEXTURE2D_H
