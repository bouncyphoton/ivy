#ifndef IVY_MODEL_H
#define IVY_MODEL_H

#include "ivy/scene/components/component.h"
#include "ivy/resources/model_resource.h"
#include "ivy/resources/resource_manager.h"

namespace ivy {

/**
 * \brief Entity component with data for rendering a mesh
 */
class Model : public Component {
public:
    explicit Model(const ModelResource &mesh_resource)
        : modelResource_(mesh_resource) {}

    [[nodiscard]] std::string getName() const override {
        return "Model";
    }

    [[nodiscard]] const std::vector<gfx::Mesh> &getMeshes(u32 lod = 0) const {
        lod = std::min(lod, ivy::ResourceManager::MAX_LOD);
        return modelResource_.get().at(lod);
    }

private:
    ModelResource modelResource_;
    // TODO: shadow options, lighting options, etc.
};

}

#endif //IVY_MODEL_H
