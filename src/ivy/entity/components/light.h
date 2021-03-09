#ifndef IVY_LIGHT_H
#define IVY_LIGHT_H

#include "ivy/entity/components/component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ivy {

// TODO: more light types

/**
 * \brief Entity component with data for a directional light source
 */
class DirectionalLight : public Component {
public:
    DirectionalLight(glm::vec3 direction, glm::vec3 color = glm::vec3(1), f32 intensity = 1.0f,
                     bool casts_shadows = true, f32 shadow_bias = 0.005f)
        : direction_(direction), color_(color), intensity_(intensity),
          castsShadows_(casts_shadows), shadowBias_(shadow_bias) {}

    [[nodiscard]] glm::vec3 getDirection() const {
        return direction_;
    }

    [[nodiscard]] glm::vec3 getColor() const {
        return color_;
    }

    [[nodiscard]] f32 getIntensity() const {
        return intensity_;
    }

    [[nodiscard]] bool castsShadows() const {
        return castsShadows_;
    }

    [[nodiscard]] f32 getShadowBias() const {
        return shadowBias_;
    }

    [[nodiscard]] glm::mat4 calculateViewProjectionMatrix(const Transform &transform, const Camera &camera) const {
        (void)camera;

        // TODO: calculate from camera frustum instead of hardcoding values
        glm::mat4 projection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 40.0f);
        glm::mat4 view = glm::lookAt(transform.getPosition() - direction_ * 20.0f, transform.getPosition(), Transform::UP);

        return projection * view;
    }

private:
    glm::vec3 direction_;
    glm::vec3 color_;
    f32 intensity_;
    bool castsShadows_;
    f32 shadowBias_;
};

}

#endif //IVY_LIGHT_H
