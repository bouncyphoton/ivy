#include "descriptor_set_cache.h"

namespace ivy::gfx {

VkDescriptorSet DescriptorSetCache::findSetWithLayout(VkDescriptorSetLayout layout) {
    std::stack<VkDescriptorSet> &availableSets = availableSetStacks_[layout];

    if (availableSets.empty()) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSet set = availableSets.top();
    availableSets.pop();
    usedSetStacks_[layout].push(set);

    return set;
}

void DescriptorSetCache::addToCache(VkDescriptorSetLayout layout, VkDescriptorSet set) {
    usedSetStacks_[layout].push(set);
}

void DescriptorSetCache::markAllAsAvailable() {
    for (auto &usedSetsPair : usedSetStacks_) {
        VkDescriptorSetLayout layout = usedSetsPair.first;

        std::stack<VkDescriptorSet> &used = usedSetsPair.second;
        std::stack<VkDescriptorSet> &available = availableSetStacks_[layout];

        while (!used.empty()) {
            available.push(used.top());
            used.pop();
        }
    }
}

size_t DescriptorSetCache::countTotalCached() {
    return countNumUsed() + countNumAvailable();
}

size_t DescriptorSetCache::countNumUsed() {
    size_t numUsed = 0;
    for (const auto &stacks : usedSetStacks_) {
        numUsed += stacks.second.size();
    }
    return numUsed;
}

size_t DescriptorSetCache::countNumAvailable() {
    size_t numAvailable = 0;
    for (const auto &stacks : availableSetStacks_) {
        numAvailable += stacks.second.size();
    }
    return numAvailable;
}

}

