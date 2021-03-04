#ifndef IVY_MODEL_RESOURCE_H
#define IVY_MODEL_RESOURCE_H

#include "ivy/graphics/command_buffer.h"
#include "ivy/graphics/geometry.h"
#include "ivy/graphics/vertex.h"

namespace ivy {

/**
 * \brief A handle for mesh datas on the GPU
 */
class ModelResource {
public:
    void draw(gfx::CommandBuffer &cmd) const;

private:
    friend class ResourceManager;

    explicit ModelResource(const std::vector<gfx::Geometry> &geometries);

    const std::vector<gfx::Geometry> &geometries_;
    // TODO: material
};

}

#endif // IVY_MODEL_RESOURCE_H
