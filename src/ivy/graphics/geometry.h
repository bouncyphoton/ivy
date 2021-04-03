#ifndef IVY_GEOMETRY_H
#define IVY_GEOMETRY_H

#include "ivy/types.h"
#include "ivy/graphics/command_buffer.h"
#include "ivy/graphics/render_device.h"
#include <vulkan/vulkan.h>

namespace ivy::gfx {

/**
 * \brief Holds geometry data for rendering
 */
class Geometry {
public:
    template<typename T> Geometry(RenderDevice &device, const std::vector<T> &vertices, const std::vector<u32> &indices)
        : numVertices_(vertices.size()), numIndices_(indices.size()) {
        // Create vertex and index buffers
        vertexBuffer_ = device.createVertexBuffer(vertices.data(), sizeof(vertices[0]) * numVertices_);
        indexBuffer_ = device.createIndexBuffer(indices.data(), sizeof(indices[0]) * numIndices_);
    }

    // Create geometry and reuse already existing vertex buffer from another geometry, useful for LODs
    Geometry(RenderDevice &device, const Geometry &vertex_src, const std::vector<u32> &indices)
        : numVertices_(vertex_src.numVertices_), numIndices_(indices.size()), vertexBuffer_(vertex_src.vertexBuffer_) {
        // Create index buffer
        indexBuffer_ = device.createIndexBuffer(indices.data(), sizeof(indices[0]) * numIndices_);
    }

    void draw(CommandBuffer &cmd) const;

private:
    u32 numVertices_;
    u32 numIndices_;

    VkBuffer vertexBuffer_;
    VkBuffer indexBuffer_;
};

}

#endif // IVY_GEOMETRY_H
