#ifndef IVY_MESH_RESOURCE_H
#define IVY_MESH_RESOURCE_H

#include "ivy/graphics/command_buffer.h"
#include "ivy/graphics/geometry.h"
#include "ivy/graphics/vertex.h"

namespace ivy {

/**
 * \brief A handle for mesh data on the GPU
 */
class MeshResource {
public:
    void draw(gfx::CommandBuffer &cmd);

private:
    friend class ResourceManager;

    explicit MeshResource(gfx::Geometry &geometry);

    gfx::Geometry &geometry_;
    // TODO: material
};

}

#endif // IVY_MESH_RESOURCE_H
