#ifndef IVY_VERTEX_H
#define IVY_VERTEX_H

#include <glm/glm.hpp>

namespace ivy::gfx {

/**
 * \brief A vertex with a position (3 components), a normal (3 components) and texture coordinates (2 components)
 */
class VertexP3N3UV2 {
public:
    VertexP3N3UV2(glm::vec3 position, glm::vec3 normal, glm::vec2 uv)
        : position_(position), normal_(normal), uv_(uv) {}

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindings;

        bindings.emplace_back();
        bindings.back().binding = 0;
        bindings.back().stride = sizeof(VertexP3N3UV2);
        bindings.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindings;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributes;

        // position
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 0;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(VertexP3N3UV2, position_);

        // normal
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 1;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(VertexP3N3UV2, normal_);

        // uv
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 2;
        attributes.back().format = VK_FORMAT_R32G32_SFLOAT;
        attributes.back().offset = offsetof(VertexP3N3UV2, uv_);

        return attributes;
    }

private:
    glm::vec3 position_;
    glm::vec3 normal_;
    glm::vec2 uv_;
};

}

#endif // IVY_VERTEX_H
