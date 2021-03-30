#include "texture.h"
#include "ivy/graphics/render_device.h"
#include "ivy/log.h"

namespace ivy::gfx {

TextureBuilder::TextureBuilder(RenderDevice &device)
    : device_(device) {}

TextureBuilder &TextureBuilder::setExtent2D(u32 width, u32 height) {
    extent_ = { width, height, 1 };
    type_ = Texture::Type::TEX_2D;
    return *this;
}

TextureBuilder &TextureBuilder::setExtentCubemap(u32 side_length) {
    extent_ = { side_length, side_length, 1 };
    type_ = Texture::Type::TEX_CUBEMAP;
    return *this;
}

TextureBuilder &TextureBuilder::setFormat(VkFormat format) {
    format_ = format;
    return *this;
}

TextureBuilder &TextureBuilder::setImageAspect(VkImageAspectFlags image_aspect) {
    imageAspect_ = image_aspect;
    return *this;
}

TextureBuilder &TextureBuilder::setAdditionalUsage(VkImageUsageFlags additional_usage) {
    additionalUsage_ = additional_usage;
    return *this;
}

TextureBuilder &TextureBuilder::setArrayLength(u32 length) {
    arrayLength_ = length;
    return *this;
}

TextureBuilder &TextureBuilder::setData(const void *data, VkDeviceSize data_size) {
    data_ = data;
    dataSize_ = data_size;
    return *this;
}

Texture TextureBuilder::build() {
    // TODO: error on invalid settings

    VkImageCreateInfo imageCI = {};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.format = format_;
    imageCI.extent = extent_;
    imageCI.mipLevels = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | additionalUsage_;
    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo viewCI = {};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.format = format_;
    viewCI.subresourceRange.aspectMask = imageAspect_;
    viewCI.subresourceRange.levelCount = imageCI.mipLevels;

    if (arrayLength_ < 1) {
        Log::fatal("Invalid array size: %", arrayLength_);
    }

    switch (type_) {
        case Texture::Type::TEX_CUBEMAP:
            imageCI.arrayLayers = 6 * arrayLength_;
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            viewCI.viewType = arrayLength_ == 1 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            break;
        case Texture::Type::TEX_2D:
            imageCI.arrayLayers = arrayLength_;
            imageCI.flags = 0;
            viewCI.viewType = arrayLength_ == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            break;
    }

    viewCI.subresourceRange.layerCount = imageCI.arrayLayers;

    std::pair<VkImage, VkImageView> imagePair = device_.createTextureGPUFromData(imageCI, viewCI, data_, dataSize_);

    return Texture(imagePair.first, imagePair.second, imageCI, viewCI);
}

}
