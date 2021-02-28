#ifndef IVY_MODEL_H
#define IVY_MODEL_H

#include "ivy/entity/components/component.h"
#include "ivy/resources/model_resource.h"
#include "ivy/graphics/command_buffer.h"

namespace ivy {

/**
 * \brief Entity component with data for rendering a mesh
 */
class Model : public Component {
public:
    Model(const ModelResource &mesh_resource)
        : meshResource_(mesh_resource) {}

    void draw(gfx::CommandBuffer &cmd) {
        meshResource_.draw(cmd);
    }

private:
    ModelResource meshResource_;
    // TODO: shadow options, lighting options, etc.
};

}

#endif //IVY_MODEL_H
