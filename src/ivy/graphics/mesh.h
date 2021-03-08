#ifndef IVY_MESH_H
#define IVY_MESH_H

#include "ivy/graphics/geometry.h"
#include "ivy/graphics/material.h"

namespace ivy::gfx {

/**
 * \brief Holds geometry and material pair for rendering
 */
class Mesh {
public:
    Mesh(const Geometry &geometry, const Material &material)
        : geometry_(geometry), material_(material) {}

    [[nodiscard]] const Geometry &getGeometry() const {
        return geometry_;
    }

    [[nodiscard]] const Material &getMaterial() const {
        return material_;
    }

private:
    Geometry geometry_;
    Material material_;

    // TODO: share same material across geometries?
};

}

#endif // IVY_MESH_H
