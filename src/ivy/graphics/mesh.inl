#include "mesh.h"
#include "ivy/utils/utils.h"

namespace ivy::gfx {

template<typename T>
MeshStatic<T>::MeshStatic(RenderDevice &device, const std::vector<T> &vertices, const std::vector<u32> &indices)
    : numVertices_(vertices.size()), numIndices_(indices.size()) {
    // Create vertex and index buffers
    vertexBuffer_ = device.createVertexBuffer(vertices.data(), sizeof(vertices[0]) * numVertices_);
    indexBuffer_ = device.createIndexBuffer(indices.data(), sizeof(indices[0]) * numIndices_);
}

template<typename T>
void MeshStatic<T>::draw(CommandBuffer &cmd) {
    cmd.bindVertexBuffer(vertexBuffer_);
    cmd.bindIndexBuffer(indexBuffer_);
    cmd.drawIndexed(numIndices_, 1, 0, 0, 0);
}

}
