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
        // TODO: full bindless

        // Create vertex buffers
        vertexBufferSize_ = sizeof(vertices[0]) * numVertices_;
        vertexBuffer_ = device.createBufferGPU(vertices.data(), vertexBufferSize_,
                                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        // Create index buffers
        indexBufferSize_ = sizeof(indices[0]) * numIndices_;
        indexBuffer_ = device.createBufferGPU(indices.data(), indexBufferSize_,
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    // Create geometry and reuse already existing vertex buffer from another geometry, useful for LODs
    Geometry(RenderDevice &device, const Geometry &vertex_src, const std::vector<u32> &indices)
        : numVertices_(vertex_src.numVertices_), numIndices_(indices.size()), vertexBuffer_(vertex_src.vertexBuffer_),
          vertexBufferSize_(vertex_src.vertexBufferSize_) {
        // Create index buffer
        indexBufferSize_ = sizeof(indices[0]) * numIndices_;
        indexBuffer_ = device.createBufferGPU(indices.data(), indexBufferSize_,
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    void draw(CommandBuffer &cmd) const;

    [[nodiscard]] u32 getNumVertices() const {
        return numVertices_;
    }

    [[nodiscard]] u32 getNumIndices() const {
        return numIndices_;
    }

    [[nodiscard]] VkBuffer getVertexBuffer() const {
        return vertexBuffer_;
    }

    [[nodiscard]] VkBuffer getIndexBuffer() const {
        return indexBuffer_;
    }

    [[nodiscard]] VkDeviceSize getVertexBufferSize() const {
        return vertexBufferSize_;
    }

    [[nodiscard]] VkDeviceSize getIndexBufferSize() const {
        return indexBufferSize_;
    }

private:
    u32 numVertices_;
    u32 numIndices_;

    VkBuffer vertexBuffer_;
    VkBuffer indexBuffer_;
    VkDeviceSize vertexBufferSize_;
    VkDeviceSize indexBufferSize_;
};

}

#endif // IVY_GEOMETRY_H
