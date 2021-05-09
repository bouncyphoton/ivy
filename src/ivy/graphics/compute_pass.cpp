#include "compute_pass.h"
#include "ivy/log.h"
#include "ivy/graphics/render_device.h"
#include "vk_utils.h"

namespace ivy::gfx {

ComputePassBuilder::ComputePassBuilder(RenderDevice &device)
    : device_(device) {}

ComputePassBuilder &ComputePassBuilder::setShader(const std::string &shader_path) {
    shader_ = Shader(Shader::StageEnum::COMPUTE, shader_path);
    return *this;
}

ComputePassBuilder &ComputePassBuilder::addUniformBufferDescriptor(u32 set, u32 binding) {
    addDescriptor(set, binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    return *this;
}

ComputePassBuilder &ComputePassBuilder::addStorageImageDescriptor(u32 set, u32 binding) {
    addDescriptor(set, binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    return *this;
}

ComputePassBuilder &ComputePassBuilder::addStorageBufferDescriptor(u32 set, u32 binding) {
    addDescriptor(set, binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    return *this;
}

ComputePass ComputePassBuilder::build() {
    Log::debug("Building compute pass");

    if (!shader_) {
        Log::fatal("Compute pass must have a shader");
    }

    // TODO: share code between graphics and compute passes

    // A table of DescriptorSetLayouts for the final ComputePass, <set, layout>
    std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> descriptorSetLayouts;

    for (const auto &descriptorSet : descriptors_) {
        u32 setIdx = descriptorSet.first;
        const std::map<u32, VkDescriptorSetLayoutBinding> &bindings = descriptorSet.second;

        // Turn bindings map into a vector
        std::vector<VkDescriptorSetLayoutBinding> bindingsVector;
        bindingsVector.reserve(bindings.size());
        for (const auto &binding : bindings) {
            bindingsVector.emplace_back(binding.second);
        }

        // Store descriptor set layout
        descriptorSetLayouts.emplace(setIdx, bindingsVector);

        // Debug logging
        Log::debug("    - set % has % bindings", setIdx, bindings.size());
        for (u32 bindingIdx = 0; bindingIdx < bindingsVector.size(); ++bindingIdx) {
            Log::debug("      - binding %: %", bindingIdx,
                       vk_descriptor_type_to_string(bindingsVector[bindingIdx].descriptorType));
        }
    }

    // Create pipeline layout
    SubpassLayout layout = device_.createLayout(descriptors_);

    // Create compute pipeline
    VkPipeline pipeline = device_.createComputePipeline(*shader_, layout.pipelineLayout);

    return ComputePass(pipeline, layout.pipelineLayout, layout.setLayouts, descriptorSetLayouts);
}

void ComputePassBuilder::addDescriptor(u32 set, u32 binding, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    descriptors_[set][binding] = layoutBinding;
}

}
