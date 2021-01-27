#ifndef IVY_DESCRIPTOR_SET_H
#define IVY_DESCRIPTOR_SET_H

#include "ivy/types.h"
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

// TODO: allow already set parts of descriptor set to be updated
// TODO: array descriptors

/**
 * \brief Used to pass descriptor set data to the command buffer
 */
class DescriptorSet {
public:
    DescriptorSet(const GraphicsPass &pass, u32 subpass_index, u32 set_index);

    void setInputAttachment(u32 binding, const std::string &attachment_name);

    void setUniformBuffer(u32 binding, const void *data, size_t size);

    template <typename T>
    void setUniformBuffer(u32 binding, const T &data) {
        setUniformBuffer(binding, &data, sizeof(T));
    }

    void validate() const;

    [[nodiscard]] u32 getSubpassIndex() const {
        return layout_.subpassIndex;
    }

    [[nodiscard]] u32 getSetIndex() const {
        return layout_.setIndex;
    }

    [[nodiscard]] const std::vector<InputAttachmentDescriptorInfo> &getInputAttachmentInfos() const {
        return inputAttachmentInfos_;
    }

    [[nodiscard]] const std::vector<UniformBufferDescriptorInfo> &getUniformBufferInfos() const {
        return uniformBufferInfos_;
    }

    [[nodiscard]] const std::vector<u8> &getUniformBufferData() const {
        return uniformBufferData_;
    }

private:
    std::string subpassName_;
    DescriptorSetLayout layout_;

    std::vector<InputAttachmentDescriptorInfo> inputAttachmentInfos_;
    std::vector<UniformBufferDescriptorInfo> uniformBufferInfos_;
    std::vector<u8> uniformBufferData_;
};

}

#endif // IVY_DESCRIPTOR_SET_H
