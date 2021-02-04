#ifndef IVY_TRANSFORM_H
#define IVY_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ivy {

class Transform {
public:
    explicit Transform(glm::vec3 position = glm::vec3(0),
                       glm::quat orientation = glm::vec3(0),
                       glm::vec3 scale = glm::vec3(1))
        : position_(position), orientation_(orientation), scale_(scale) {}

    [[nodiscard]] glm::mat4 getModelMatrix() const {
        return glm::translate(glm::mat4(1), position_) *
               glm::mat4_cast(orientation_) *
               glm::scale(glm::mat4(1), scale_);
    }

    [[nodiscard]] glm::vec3 getPosition() const {
        return position_;
    }

    [[nodiscard]] glm::quat getOrientation() const {
        return orientation_;
    }

    [[nodiscard]] glm::vec3 getScale() const {
        return scale_;
    }

    void setPosition(glm::vec3 position) {
        position_ = position;
    }

    void setOrientation(glm::quat orientation) {
        orientation_ = orientation;
    }

    void setScale(glm::vec3 scale) {
        scale_ = scale;
    }

private:
    glm::vec3 position_;
    glm::quat orientation_;
    glm::vec3 scale_;
};

}

#endif //IVY_TRANSFORM_H
