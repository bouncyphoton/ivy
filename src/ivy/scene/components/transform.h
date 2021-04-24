#ifndef IVY_TRANSFORM_H
#define IVY_TRANSFORM_H

#include "ivy/scene/components/component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ivy {

/**
 * \brief Entity component with transformation data
 */
class Transform : public Component {
public:
    explicit Transform(glm::vec3 position = glm::vec3(0),
                       glm::vec3 rotation = glm::vec3(0),
                       glm::vec3 scale = glm::vec3(1))
        : position_(position), rotation_(rotation), scale_(scale) {}

    [[nodiscard]] std::string getName() const override {
        return "Transform";
    }

    [[nodiscard]] glm::mat4 getModelMatrix() const {
        return glm::translate(glm::mat4(1), position_) *
               glm::mat4_cast(getOrientation()) *
               glm::scale(glm::mat4(1), scale_);
    }

    [[nodiscard]] glm::vec3 getPosition() const {
        return position_;
    }

    [[nodiscard]] glm::quat getOrientation() const {
        return glm::quat(getRotation());
    }

    [[nodiscard]] glm::vec3 getRotation() const {
        return rotation_;
    }

    [[nodiscard]] glm::vec3 getScale() const {
        return scale_;
    }

    [[nodiscard]] glm::vec3 getForward() const {
        return getOrientation() * FORWARD;
    }

    [[nodiscard]] glm::vec3 getUp() const {
        return getOrientation() * UP;
    }

    [[nodiscard]] glm::vec3 getRight() const {
        return getOrientation() * RIGHT;
    }

    void setPosition(glm::vec3 position) {
        position_ = position;
    }

    void setOrientation(glm::quat orientation) {
        setRotation(glm::eulerAngles(orientation));
    }

    void setRotation(glm::vec3 rotation) {
        rotation_ = glm::mod(rotation, glm::vec3(glm::two_pi<f32>()));
    }

    void addRotation(glm::vec3 rotation) {
        setRotation(getRotation() + rotation);
    }

    void setScale(glm::vec3 scale) {
        scale_ = scale;
    }

    void addPosition(glm::vec3 position) {
        setPosition(getPosition() + position);
    }

    constexpr static glm::vec3 FORWARD = glm::vec3(0, 0, -1);
    constexpr static glm::vec3 UP      = glm::vec3(0, 1, 0);
    constexpr static glm::vec3 RIGHT   = glm::vec3(1, 0, 0);

private:
    glm::vec3 position_;
    glm::vec3 rotation_;
    glm::vec3 scale_;
};

}

#endif //IVY_TRANSFORM_H
