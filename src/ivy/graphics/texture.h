#ifndef IVY_TEXTURE_H
#define IVY_TEXTURE_H

#include "ivy/types.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace ivy::gfx {

class RenderDevice;

class Texture {
public:
    enum class Type {
        TEX_2D, TEX_CUBEMAP
    };

    [[nodiscard]] VkImage getImage() const {
        return image_;
    }

    [[nodiscard]] VkImageView getImageView() const {
        return imageView_;
    }

    [[nodiscard]] VkExtent3D getExtent() const {
        return imageCI_.extent;
    }

    [[nodiscard]] u32 getLayers() const {
        return imageCI_.arrayLayers;
    }

    [[nodiscard]] VkImageViewType getViewType() const {
        return viewCI_.viewType;
    }

    [[nodiscard]] VkFormat getFormat() const {
        return imageCI_.format;
    }

    [[nodiscard]] VkImageAspectFlags getImageAspect() const {
        return viewCI_.subresourceRange.aspectMask;
    }

private:
    friend class TextureBuilder;

    Texture(VkImage image, VkImageView image_view, VkImageCreateInfo image_ci, VkImageViewCreateInfo view_ci)
        : image_(image), imageView_(image_view), imageCI_(image_ci), viewCI_(view_ci) {}

    VkImage image_;
    VkImageView imageView_;
    VkImageCreateInfo imageCI_;
    VkImageViewCreateInfo viewCI_;
};

class TextureBuilder {
public:
    explicit TextureBuilder(RenderDevice &device);

    // TODO: load/store ops

    TextureBuilder &setExtent2D(u32 width, u32 height);

    TextureBuilder &setExtentCubemap(u32 side_length);

    TextureBuilder &setFormat(VkFormat format);

    // TODO: deduce from format?
    TextureBuilder &setImageAspect(VkImageAspectFlags image_aspect);

    TextureBuilder &setAdditionalUsage(VkImageUsageFlags additional_usage);

    TextureBuilder &setArrayLength(u32 length);

    TextureBuilder &setData(const void *data, VkDeviceSize data_size);

    template <typename T>
    TextureBuilder &setData(const std::vector<T> &data) {
        setData(data.data(), sizeof(T) * data.size());
        return *this;
    }

    Texture build();

private:
    RenderDevice &device_;

    Texture::Type type_;
    VkExtent3D extent_{};
    VkFormat format_{};
    VkImageAspectFlags imageAspect_{};
    VkSampler sampler_{};
    VkImageUsageFlags additionalUsage_{};
    u32 arrayLength_ = 1;
    const void *data_{};
    VkDeviceSize dataSize_{};
};

}

#endif // IVY_TEXTURE_H
