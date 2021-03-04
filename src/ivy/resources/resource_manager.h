#ifndef IVY_RESOURCE_MANAGER_H
#define IVY_RESOURCE_MANAGER_H

#include "ivy/resources/model_resource.h"
#include <memory>

namespace ivy {

class ResourceManager {
public:
    explicit ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory);

    ModelResource getModel(const std::string &model_name);

private:
    /**
     * \brief Load a model into the resource manager
     * \param resource_path The path to the resource relative to the resource directory
     * \return Whether or not the model was loaded sucessfully
     */
    bool loadModelFromFile(const std::string &resource_path);

    void loadModel(const std::string &name, const std::vector<gfx::Geometry> &meshes);

    gfx::RenderDevice &device_;
    std::string resourceDirectory_;

    std::unordered_map<std::string, std::unique_ptr<std::vector<gfx::Geometry>>> modelDatas_;
};

}

#endif // IVY_RESOURCE_MANAGER_H
