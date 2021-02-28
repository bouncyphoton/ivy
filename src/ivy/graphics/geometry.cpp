#include "geometry.h"

namespace ivy::gfx {

void Geometry::draw(CommandBuffer &cmd) const {
    cmd.bindVertexBuffer(vertexBuffer_);
    cmd.bindIndexBuffer(indexBuffer_);
    cmd.drawIndexed(numIndices_, 1, 0, 0, 0);
}

}
