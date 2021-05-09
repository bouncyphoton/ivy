#include "descriptor_set.h"
#include "ivy/log.h"
#include "ivy/graphics/graphics_pass.h"
#include "ivy/graphics/compute_pass.h"
#include "ivy/graphics/vk_utils.h"
#include <set>
#include <unordered_map>
#include <cstring>

namespace ivy::gfx {

// TODO: theres a lot of repetitive code, can be generalized

DescriptorSet::DescriptorSet(const GraphicsPass &pass, u32 subpass_index, u32 set_index)
    : subpassIndex_(subpass_index), setIndex_(set_index),
      bindings_(pass.getDescriptorSetLayoutBindings(subpass_index, set_index)) {
}

DescriptorSet::DescriptorSet(const ComputePass &pass, u32 set_index)
    : subpassIndex_(0), setIndex_(set_index), bindings_(pass.getDescriptorSetLayoutBindings(set_index)) {
}

void DescriptorSet::setInputAttachment(u32 binding, const std::string &attachment_name) {
    inputAttachmentInfos_.emplace_back(binding, attachment_name);
}

void DescriptorSet::setUniformBuffer(u32 binding, const void *data, size_t size) {
    u32 offset = uniformBufferData_.size();
    u32 range = size;

    uniformBufferData_.resize(offset + range);
    std::memcpy(uniformBufferData_.data() + offset, data, range);
    uniformBufferInfos_.emplace_back(binding, offset, range);
}

void DescriptorSet::setTexture(u32 binding, const Texture &texture, VkSampler sampler) {
    setTexture(binding, texture.getImageView(), sampler);
}

void DescriptorSet::setTexture(u32 binding, VkImageView view, VkSampler sampler) {
    combinedImageSamplerInfos_.emplace_back(binding, view, sampler);
}

void DescriptorSet::setStorageImage(u32 binding, const Texture &texture) {
    setStorageImage(binding, texture.getImageView());
}

void DescriptorSet::setStorageImage(u32 binding, VkImageView view) {
    storageImageSamplerInfos_.emplace_back(binding, view);
}

void DescriptorSet::setStorageBuffer(u32 binding, VkBuffer buffer, VkDeviceSize size) {
    storageBufferInfos_.emplace_back(binding, buffer, size);
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
            errorMessage += "\n- Binding " + std::to_string(binding) + " was not described in graphics pass";
        } else if (validBindings[binding] != type) {
            // If this binding has the incorrect descriptor type, that's an error
            errorMessage += "\n- Binding " + std::to_string(binding) + " should be of type " +
                            vk_descriptor_type_to_string(validBindings[binding]);
        }

        // Mark that we've seen this binding
        seenBindings.emplace(binding);
    };

    // Store valid bindings for easy look up
    for (const VkDescriptorSetLayoutBinding &binding : bindings_) {
        validBindings[binding.binding] = binding.descriptorType;
    }

    // Check input attachment bindings
    for (const InputAttachmentDescriptorInfo &info : inputAttachmentInfos_) {
        validateBinding(info.binding, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    }

    // Check buffer bindings
    for (const UniformBufferDescriptorInfo &info : uniformBufferInfos_) {
        validateBinding(info.binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    // Check combined image sampler bindings
    for (const CombinedImageSamplerDescriptorInfo &info : combinedImageSamplerInfos_) {
        validateBinding(info.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    // Check storage images
    for (const StorageImageSamplerDescriptorInfo &info : storageImageSamplerInfos_) {
        validateBinding(info.binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }

    // Check storage buffer
    for (const StorageBufferDescriptorInfo &info : storageBufferInfos_) {
        validateBinding(info.binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
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
        Log::fatal("Descriptor set % in pass % is invalid: %", setIndex_, subpassIndex_, errorMessage);
    }
}

}
