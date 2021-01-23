#include "descriptor_set.h"
#include "ivy/log.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/vk_utils.h"
#include <set>
#include <unordered_map>

namespace ivy::gfx {

DescriptorSet::DescriptorSet(const GraphicsPass &pass, u32 subpass_index, u32 set_index)
    : subpassName_(pass.getSubpass(subpass_index).getName()),
      layout_(pass.getDescriptorSetLayout(subpass_index, set_index)) {
}

void DescriptorSet::setInputAttachment(u32 binding, const std::string &attachment_name) {
    inputAttachmentInfos_.emplace_back(binding, attachment_name);
}

void DescriptorSet::validate() const {
    std::string errorMessage;
    std::set<u32> seenBindings;
    std::unordered_map<u32, VkDescriptorType> validBindings;

    auto validateBinding = [&](u32 binding, VkDescriptorType type) {
        if (seenBindings.find(binding) != seenBindings.end()) {
            // If we've already seen this binding, that's an error
            errorMessage += "\n- Binding " + std::to_string(binding) + " was written to multiple times";
            return;
        }

        if (validBindings.find(binding) == validBindings.end()) {
            // If this binding is not in the valid bindings set, that's an error
            errorMessage += "\n- Binding " + std::to_string(binding) + " is invalid";
        } else if (validBindings[binding] != type) {
            // If this binding has the incorrect descriptor type, that's an error
            errorMessage += "\n- Binding " + std::to_string(binding) + " should be of type " +
                            vk_descriptor_type_to_string(validBindings[binding]);
        }

        // Mark that we've seen this binding
        seenBindings.emplace(binding);
    };

    // Store valid bindings for easy look up
    for (const VkDescriptorSetLayoutBinding &binding : layout_.bindings) {
        validBindings[binding.binding] = binding.descriptorType;
    }

    // Check input attachment bindings
    for (const InputAttachmentDescriptorInfo &info : inputAttachmentInfos_) {
        validateBinding(info.binding, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    }
    // TODO: check other bindings when they get implemented

    // If we've seen fewer bindings than there are valid bindings, we're missing at least one
    if (seenBindings.size() < validBindings.size()) {
        // Find missing bindings
        for (auto bindingTypePair : validBindings) {
            u32 binding = bindingTypePair.first;
            VkDescriptorType type = bindingTypePair.second;

            if (seenBindings.find(binding) == seenBindings.end()) {
                errorMessage += "\n- Binding " + std::to_string(binding) + " (" +
                                vk_descriptor_type_to_string(type) + ") was not written to";
            }
        }
    }

    if (!errorMessage.empty()) {
        Log::fatal("Descriptor set %d for subpass %d (%s) is invalid:%s", layout_.setIndex, layout_.subpassIndex,
                   subpassName_.c_str(), errorMessage.c_str());
    }
}

}
