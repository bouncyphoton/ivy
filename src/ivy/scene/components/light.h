#ifndef IVY_LIGHT_H
#define IVY_LIGHT_H

#include "ivy/scene/components/component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ivy {

// TODO: more light types

enum LightType {
    DIRECTIONAL,
    POINT
};

/**
 * \brief Entity component with data for a directional light source
 */
class DirectionalLight : public Component {
public:
    DirectionalLight(glm::vec3 direction, glm::vec3 color = glm::vec3(1), f32 intensity = 1.0f,
                     bool casts_shadows = true, f32 shadow_bias = 0.005f)
        : direction_(direction), color_(color), intensity_(intensity),
          castsShadows_(casts_shadows), shadowBias_(shadow_bias) {}

    [[nodiscard]] std::string getName() const override {
        return "DirectionalLight";
    }

    [[nodiscard]] glm::vec3 getDirection() const {
        return direction_;
    }

    void setDirection(glm::vec3 direction) {
        direction_ = direction;
    }

    [[nodiscard]] glm::vec3 getColor() const {
        return color_;
    }

    void setColor(glm::vec3 color) {
        color_ = color;
    }

    [[nodiscard]] f32 getIntensity() const {
        return intensity_;
    }

    void setIntensity(f32 intensity) {
        intensity_ = intensity;
    }

    [[nodiscard]] bool castsShadows() const {
        return castsShadows_;
    }

    void setShadowCasting(bool casts_shadows) {
        castsShadows_ = casts_shadows;
    }

    [[nodiscard]] f32 getShadowBias() const {
        return shadowBias_;
    }

    void setShadowBias(float bias) {
        shadowBias_ = bias;
    }

    [[nodiscard]] glm::mat4 calculateViewProjectionMatrix(const Transform &transform,
                                                          [[maybe_unused]] const Camera &camera) const {
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

// TODO: there is a lot of shared functionality, maybe create a light base class?

class PointLight : public Component {
public:
    explicit PointLight(glm::vec3 color = glm::vec3(1), f32 intensity = 1.0f,
                        bool casts_shadows = true, f32 shadow_bias = 0.005f, f32 near_plane = 0.01f, f32 far_plane = 100.0f)
        : color_(color), intensity_(intensity), castsShadows_(casts_shadows), shadowBias_(shadow_bias),
          nearPlane_(near_plane), farPlane_(far_plane) {}

    [[nodiscard]] std::string getName() const override {
        return "PointLight";
    }

    [[nodiscard]] glm::vec3 getColor() const {
        return color_;
    }

    void setColor(glm::vec3 color) {
        color_ = color;
    }

    [[nodiscard]] f32 getIntensity() const {
        return intensity_;
    }

    void setIntensity(f32 intensity) {
        intensity_ = intensity;
    }

    [[nodiscard]] bool castsShadows() const {
        return castsShadows_;
    }

    void setShadowCasting(bool casts_shadows) {
        castsShadows_ = casts_shadows;
    }

    [[nodiscard]] f32 getShadowBias() const {
        return shadowBias_;
    }

    void setShadowBias(float bias) {
        shadowBias_ = bias;
    }

    [[nodiscard]] f32 getNearPlane() const {
        return nearPlane_;
    }

    void setNearPlane(f32 near_plane) {
        nearPlane_ = near_plane;
    }

    [[nodiscard]] f32 getFarPlane() const {
        return farPlane_;
    }

    void setFarPlane(f32 far_plane) {
        farPlane_ = far_plane;
    }

private:
    glm::vec3 color_;
    f32 intensity_;
    bool castsShadows_;
    f32 shadowBias_;
    f32 nearPlane_;
    f32 farPlane_;
};

}

#endif //IVY_LIGHT_H
