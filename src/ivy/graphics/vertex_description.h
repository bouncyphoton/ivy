#ifndef IVY_VERTEX_DESCRIPTION_H
#define IVY_VERTEX_DESCRIPTION_H

#include <vulkan/vulkan.h>
#include <vector>

namespace ivy::gfx {

/**
 * \brief A wrapper around vertex description data for Vulkan
 */
class VertexDescription {
public:
    VertexDescription(const std::vector <VkVertexInputBindingDescription> &bindings = {},
                      const std::vector <VkVertexInputAttributeDescription> &attributes = {}) :
        bindings_(bindings), attributes_(attributes) {}

    [[nodiscard]] const std::vector <VkVertexInputBindingDescription> &getBindings() const {
        return bindings_;
    }

    [[nodiscard]] const std::vector <VkVertexInputAttributeDescription> &getAttributes() const {
        return attributes_;
    }

private:
    std::vector <VkVertexInputBindingDescription> bindings_;
    std::vector <VkVertexInputAttributeDescription> attributes_;
};

}

#endif //IVY_VERTEX_DESCRIPTION_H
