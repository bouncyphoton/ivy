#include "texture2d.h"
#include "ivy/graphics/render_device.h"

namespace ivy::gfx {

Texture2D::Texture2D(RenderDevice &device, u32 width, u32 height, VkFormat format, const u8 *data, u32 size) {
    std::pair<VkImage, VkImageView> imagePair = device.createTextureGPUFromData(width, height, format, data, size);
    image_ = imagePair.first;
    imageView_ = imagePair.second;
    sampler_ = device.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
}

}
