#ifndef IVY_MESH_H
#define IVY_MESH_H

#include "ivy/types.h"
#include "ivy/graphics/command_buffer.h"
#include <vulkan/vulkan.h>

namespace ivy::gfx {

/**
 * \brief Represents a static list of vertices for rendering
 * \tparam T Vertex data type
 */
template<typename T>
class MeshStatic {
public:
    MeshStatic(RenderDevice &device, const std::vector<T> &vertices, const std::vector<u32> &indices);

    void draw(CommandBuffer &cmd);

private:
    u32 numVertices_;
    u32 numIndices_;

    VkBuffer vertexBuffer_;
    VkBuffer indexBuffer_;
};

}

#include "mesh.inl"

#endif // IVY_MESH_H
