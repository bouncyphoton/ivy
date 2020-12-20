#ifndef IVY_VERTEX_H
#define IVY_VERTEX_H

#include "ivy/types.h"

namespace ivy::gfx {

struct VertexP3C3 {
    // TODO: glm
    f32 p1, p2, p3;
    f32 c1, c2, c3;

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
        attributes.back().offset = offsetof(VertexP3C3, p1);

        // color
        attributes.emplace_back();
        attributes.back().binding = 0;
        attributes.back().location = 1;
        attributes.back().format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes.back().offset = offsetof(VertexP3C3, c1);

        return attributes;
    }
};

}

#endif // IVY_VERTEX_H
