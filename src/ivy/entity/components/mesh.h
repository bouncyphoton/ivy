#ifndef IVY_MESH_H
#define IVY_MESH_H

#include "ivy/entity/components/component.h"
#include "ivy/resources/mesh_resource.h"
#include "ivy/graphics/command_buffer.h"

namespace ivy {

/**
 * \brief Entity component with data for rendering a mesh
 */
class Mesh : public Component {
public:
    Mesh(const MeshResource &mesh_resource)
        : meshResource_(mesh_resource) {}

    void draw(gfx::CommandBuffer &cmd) {
        meshResource_.draw(cmd);
    }

private:
    MeshResource meshResource_;
    // TODO: shadow options, lighting options, etc.
};

}

#endif //IVY_MESH_H
