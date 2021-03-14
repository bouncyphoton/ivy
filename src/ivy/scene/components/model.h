#ifndef IVY_MODEL_H
#define IVY_MODEL_H

#include "ivy/scene/components/component.h"
#include "ivy/resources/model_resource.h"

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

    [[nodiscard]] const std::vector<gfx::Mesh> &getMeshes() const {
        return modelResource_.get();
    }

private:
    ModelResource modelResource_;
    // TODO: shadow options, lighting options, etc.
};

}

#endif //IVY_MODEL_H
