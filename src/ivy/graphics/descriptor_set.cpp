#include "descriptor_set.h"
#include "ivy/log.h"
#include "ivy/graphics/vk_utils.h"
#include <set>
#include <unordered_map>

namespace ivy::gfx {

void DescriptorSet::setInputAttachment(u32 binding, const std::string &attachment_name) {
    inputAttachmentInfos_.emplace_back(binding, attachment_name);
}

void DescriptorSet::validate() const {
    return; // TODO descriptor set validate
    std::string errorMessage;
    std::set<u32> seenBindings;
    std::unordered_map<u32, VkDescriptorType> validBindings;

    // Store valid bindings for easy look up
    for (const VkDescriptorSetLayoutBinding &binding : layout_.bindings) {
        validBindings[binding.binding] = binding.descriptorType;
    }

    // TODO: deal with this new system
//    for (const VkWriteDescriptorSet &write : writes_) {
//        if (seenBindings.find(write.dstBinding) != seenBindings.end()) {
//            // If we've already seen this binding, that's an error
//            errorMessage += "\n- Binding " + std::to_string(write.dstBinding) + " was written to multiple times";
//            continue;
//        }
//
//        if (validBindings.find(write.dstBinding) == validBindings.end()) {
//            // If this binding is not in the valid bindings set, that's an error
//            errorMessage += "\n- Binding " + std::to_string(write.dstBinding) + " is invalid";
//        } else if (validBindings[write.dstBinding] != write.descriptorType) {
//            // If this binding has the incorrect descriptor type, that's an error
//            errorMessage += "\n- Binding " + std::to_string(write.dstBinding) + " should be of type " +
//                            vk_descriptor_type_to_string(validBindings[write.dstBinding]);
//        }
//
//        // Mark that we've seen this binding
//        seenBindings.emplace(write.dstBinding);
//    }


    // If we've seen fewer bindings than there are valid bindings, we're missing at least one
    if (u32 delta = validBindings.size() - seenBindings.size()) {
        errorMessage += "\n- " + std::to_string(delta) + " binding" +
                        (delta == 1 ? " was" : "s were") + " not written to";
    }

    if (!errorMessage.empty()) {
        Log::fatal("Descriptor set validation failed:%s", errorMessage.c_str());
    }
}

}
