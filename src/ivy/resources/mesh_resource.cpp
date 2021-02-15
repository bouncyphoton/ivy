#include "mesh_resource.h"

namespace ivy {

MeshResource::MeshResource(gfx::Geometry &geometry)
    : geometry_(geometry) {}

void MeshResource::draw(gfx::CommandBuffer &cmd) {
    geometry_.draw(cmd);
}

}
