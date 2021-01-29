#ifndef IVY_VERTEX_H
#define IVY_VERTEX_H

#include <glm/glm.hpp>

namespace ivy::gfx {

/**
 * \brief A vertex with a position (3 components) and color (3 components)
 */
class VertexP3C3 {
public:
    VertexP3C3(glm::vec3 position, glm::vec3 color)
        : position_(position), color_(color) {}

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindings;

        bindings.emplace_back();
        bindings.back().binding = 0;
        bindings.back().stride = sizeof(VertexP3C3);
        bindings.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindings;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributes;

        // pos
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 0;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(VertexP3C3, position_);

        // color
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 1;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(VertexP3C3, color_);

        return attributes;
    }

private:
    glm::vec3 position_;
    glm::vec3 color_;
};

}

#endif // IVY_VERTEX_H
