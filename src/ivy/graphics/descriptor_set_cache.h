#ifndef IVY_DESCRIPTOR_SET_CACHE_H
#define IVY_DESCRIPTOR_SET_CACHE_H

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <stack>

namespace ivy::gfx {

class RenderDevice;

class DescriptorSetCache {
public:

    /**
     * \brief Find an available descriptor set with a given layout in the cache
     * \param layout The layout the descriptor set should have
     * \return The descriptor set if found, otherwise VK_NULL_HANDLE
     */
    VkDescriptorSet findSetWithLayout(VkDescriptorSetLayout layout);

    /**
     * \brief Add a descriptor set of a given layout to the cache
     * \param layout The layout of the descriptor set
     * \param set The descriptor set
     */
    void addToCache(VkDescriptorSetLayout layout, VkDescriptorSet set);

    /**
     * \brief Mark all descriptor sets in the cache as available
     */
    void markAllAsAvailable();

private:
    std::unordered_map<VkDescriptorSetLayout, std::stack<VkDescriptorSet>> availableSetStacks_;
    std::unordered_map<VkDescriptorSetLayout, std::stack<VkDescriptorSet>> usedSetStacks_;
};

}

#endif // IVY_DESCRIPTOR_SET_CACHE_H
