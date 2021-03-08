#ifndef IVY_MODEL_RESOURCE_H
#define IVY_MODEL_RESOURCE_H

#include "ivy/resources/resource.h"
#include "ivy/graphics/mesh.h"
#include <vector>

namespace ivy {

using ModelResource = Resource<std::vector<gfx::Mesh>>;

}

#endif //IVY_MODEL_RESOURCE_H
