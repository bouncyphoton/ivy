#include "model_resource.h"

namespace ivy {

ModelResource::ModelResource(const std::vector<gfx::Geometry> &geometries)
    : geometries_(geometries) {}

void ModelResource::draw(gfx::CommandBuffer &cmd) const {
    for (const gfx::Geometry &mesh : geometries_) {
        mesh.draw(cmd);
    }
}

}
