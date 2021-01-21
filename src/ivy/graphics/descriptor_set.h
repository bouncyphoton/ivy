#ifndef IVY_DESCRIPTOR_SET_H
#define IVY_DESCRIPTOR_SET_H

#include "ivy/types.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace ivy::gfx {

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

// TODO: array descriptors

/**
 * \brief Used to pass descriptor set data to the command buffer
 */
class DescriptorSet {
public:
    DescriptorSet(const DescriptorSetLayout &layout)
        : layout_(layout) {}

    void setInputAttachment(u32 binding, const std::string &attachment_name);

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

private:
    DescriptorSetLayout layout_;
    std::vector<InputAttachmentDescriptorInfo> inputAttachmentInfos_;
};

}

#endif // IVY_DESCRIPTOR_SET_H
