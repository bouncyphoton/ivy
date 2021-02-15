#ifndef IVY_RESOURCE_MANAGER_H
#define IVY_RESOURCE_MANAGER_H

#include "ivy/resources/mesh_resource.h"

namespace ivy {

class ResourceManager {
public:
    explicit ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory);

    void loadResource(const std::string &resource_path);

    // TODO: there are multiple meshes per model

    MeshResource getMesh(const std::string &mesh_name);

private:
    gfx::RenderDevice &device_;
    std::string resourceDirectory_;

    std::unordered_map<std::string, gfx::Geometry> meshDatas_;
};

}

#endif // IVY_RESOURCE_MANAGER_H
