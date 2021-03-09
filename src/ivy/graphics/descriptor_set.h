#ifndef IVY_DESCRIPTOR_SET_H
#define IVY_DESCRIPTOR_SET_H

#include "ivy/types.h"
#include "ivy/graphics/texture2d.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace ivy::gfx {

class GraphicsPass;

struct DescriptorSetLayout {
    DescriptorSetLayout(u32 subpass_index, u32 set_index, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
        : subpassIndex(subpass_index), setIndex(set_index), bindings(bindings) {}

    u32 subpassIndex;
    u32 setIndex;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct InputAttachmentDescriptorInfo {
    InputAttachmentDescriptorInfo(u32 binding, const std::string &attachment_name)
        : binding(binding), attachmentName(attachment_name) {}

    u32 binding;
    std::string attachmentName;
};

struct UniformBufferDescriptorInfo {
    UniformBufferDescriptorInfo(u32 binding, u32 data_offset, u32 data_range)
        : binding(binding), dataOffset(data_offset), dataRange(data_range) {}

    u32 binding;
    u32 dataOffset;
    u32 dataRange;
};

struct CombinedImageSamplerDescriptorInfo {
    CombinedImageSamplerDescriptorInfo(u32 binding, VkImageView view, VkSampler sampler)
        : binding(binding), view(view), sampler(sampler) {}

    u32 binding;
    VkImageView view;
    VkSampler sampler;
};

// TODO: allow already set parts of descriptor set to be updated
// TODO: array descriptors

/**
 * \brief Used to pass descriptor set data to the command buffer
 */
class DescriptorSet {
public:
    DescriptorSet(const GraphicsPass &pass, u32 subpass_index, u32 set_index);

    /**
     * \brief Set an input attachment in the descriptor set
     * \param binding The binding in the set for the input attachment
     * \param attachment_name The name of the attachment in the graphics pass
     */
    void setInputAttachment(u32 binding, const std::string &attachment_name);

    /**
     * \brief Set a uniform buffer in the descriptor set
     * \param binding The binding in the set for the uniform buffer
     * \param data A pointer to the data to copy into the uniform buffer
     * \param size Number of bytes
     */
    void setUniformBuffer(u32 binding, const void *data, size_t size);

    /**
     * \brief Set a uniform buffer in the descriptor set
     * \tparam T The datatype for the data
     * \param binding The binding in the set for the uniform buffer
     * \param data The data to copy into the uniform buffer
     */
    template <typename T>
    void setUniformBuffer(u32 binding, const T &data) {
        setUniformBuffer(binding, &data, sizeof(T));
    }

    /**
     * \brief Set a 2D texture in the descriptor set
     * \param binding The binding in the set for the texture
     * \param texture The texture
     */
    void setTexture(u32 binding, const Texture2D &texture);

    /**
     * \brief Set a 2D texture in the descriptor set manually
     * \param binding The binding in the set for the texture
     * \param view The image view for the texture
     * \param sampler The sampler for the texture
     */
    void setTexture(u32 binding, VkImageView view, VkSampler sampler);

    /**
     * \brief Validate that everything was set properly
     */
    void validate() const;

    /**
     * \brief Get the subpass index for this descriptor set
     * \return Subpass index
     */
    [[nodiscard]] u32 getSubpassIndex() const {
        return layout_.subpassIndex;
    }

    /**
     * \brief Get the set index in the subpass for this descriptor set
     * \return Set index
     */
    [[nodiscard]] u32 getSetIndex() const {
        return layout_.setIndex;
    }

    /**
     * \brief Get the input attachment infos for this descriptor set
     * \return Vector of InputAttachmentDescriptorInfo
     */
    [[nodiscard]] const std::vector<InputAttachmentDescriptorInfo> &getInputAttachmentInfos() const {
        return inputAttachmentInfos_;
    }

    /**
     * \brief Get the uniform buffer infos for this descriptor set
     * \return Vector of UniformBufferDescriptorInfo
     */
    [[nodiscard]] const std::vector<UniformBufferDescriptorInfo> &getUniformBufferInfos() const {
        return uniformBufferInfos_;
    }

    /**
     * \brief Get the combined image sampler infos for this descriptor set
     * \return Vector of CombinedImageSamplerDescriptorInfo
     */
    [[nodiscard]] const std::vector<CombinedImageSamplerDescriptorInfo> &getCombinedImageSamplerInfos() const {
        return combinedImageSamplerInfos_;
    }

    /**
     * \brief Get the uniform buffer data
     * \return A vector of bytes with uniform buffer data
     */
    [[nodiscard]] const std::vector<u8> &getUniformBufferData() const {
        return uniformBufferData_;
    }

private:
    std::string subpassName_;
    DescriptorSetLayout layout_;

    std::vector<InputAttachmentDescriptorInfo> inputAttachmentInfos_;
    std::vector<UniformBufferDescriptorInfo> uniformBufferInfos_;
    std::vector<CombinedImageSamplerDescriptorInfo> combinedImageSamplerInfos_;
    std::vector<u8> uniformBufferData_;
};

}

#endif // IVY_DESCRIPTOR_SET_H
