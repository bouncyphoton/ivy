#ifndef IVY_CAMERA_H
#define IVY_CAMERA_H

#include "ivy/entity/components/component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ivy {

/**
 * \brief Entity component with camera data
 */
class Camera : public Component {
public:
    explicit Camera(f32 fov_y = glm::half_pi<f32>(), f32 near_plane = 0.1f, f32 far_plane = 100.0f)
        : fovY_(fov_y), nearPlane_(near_plane), farPlane_(far_plane) {}

    [[nodiscard]] f32 getFovY() const {
        return fovY_;
    }

    [[nodiscard]] f32 getNearPlane() const {
        return nearPlane_;
    }

    [[nodiscard]] f32 getFarPlane() const {
        return farPlane_;
    }

    void setFovY(f32 fov_y) {
        fovY_ = fov_y;
    }

    void setNearPlane(f32 near_plane) {
        nearPlane_ = near_plane;
    }

    void setFarPlane(f32 far_plane) {
        farPlane_ = far_plane;
    }

private:
    f32 fovY_;
    f32 nearPlane_;
    f32 farPlane_;
};

}

#endif //IVY_CAMERA_H
