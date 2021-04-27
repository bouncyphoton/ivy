#ifndef IVY_COMPUTE_PASS_H
#define IVY_COMPUTE_PASS_H

#include "ivy/types.h"
#include "ivy/graphics/shader.h"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace ivy::gfx {

class RenderDevice;

class ComputePass {
public:
    ComputePass(VkPipeline pipeline, VkPipelineLayout pipeline_layout,
                const std::vector<VkDescriptorSetLayout> &set_layouts,
                const std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> &descriptor_set_layouts)
        : pipeline_(pipeline), pipelineLayout_(pipeline_layout), setLayouts_(set_layouts),
          descriptorSetLayouts_(descriptor_set_layouts) {}

    [[nodiscard]] VkPipeline getPipeline() const {
        return pipeline_;
    }

    [[nodiscard]] VkPipelineLayout getPipelineLayout() const {
        return pipelineLayout_;
    }

    [[nodiscard]] VkDescriptorSetLayout getSetLayout(u32 set_idx) const {
        return setLayouts_.at(set_idx);
    }

    /**
     * \brief Get the descriptor set layout bindings for a given set
     * \param set_index The set index
     * \return std::vector<VkDescriptorSetLayoutBinding>
     */
    [[nodiscard]] const std::vector<VkDescriptorSetLayoutBinding> &getDescriptorSetLayoutBindings(u32 set_index) const {
        return descriptorSetLayouts_.at(set_index);
    }

private:
    VkPipeline pipeline_;
    VkPipelineLayout pipelineLayout_;
    std::vector<VkDescriptorSetLayout> setLayouts_;
    std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> descriptorSetLayouts_;
};

class ComputePassBuilder {
public:
    explicit ComputePassBuilder(RenderDevice &device);

    ComputePassBuilder &setShader(const std::string &shader_path);

    ComputePassBuilder &addStorageImageDescriptor(u32 set, u32 binding);

    ComputePass build();

private:
    /**
     * \brief Add a descriptor to the compute pass
     * \param set The descriptor set index
     * \param binding The binding in the descriptor set
     * \param type What type of descriptor
     */
    void addDescriptor(u32 set, u32 binding, VkDescriptorType type);

    RenderDevice &device_;
    std::optional<Shader> shader_;
    std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> descriptors_;
};

}

#endif // IVY_COMPUTE_PASS_H
