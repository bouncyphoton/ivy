#ifndef IVY_PLATFORM_H
#define IVY_PLATFORM_H

#include "ivy/types.h"
#include "ivy/platform/input_state.h"

typedef struct GLFWwindow GLFWwindow;

namespace ivy {

struct Options;

/**
 * \brief Platform abstraction layer
 */
class Platform final {
public:
    explicit Platform(const Options &options);
    ~Platform();

    /**
     * \brief Poll events, update platform specific code
     */
    void update();

    [[nodiscard]] f32 getDt() const {
        return dt_;
    }

    [[nodiscard]] const InputState &getInputState() const {
        return inputState_;
    }

    /**
     * \brief Check whether platform has requested for the application to close
     * \return Whether or not close was requested
     */
    [[nodiscard]] bool isCloseRequested() const;

    /**
     * \brief Get the pointer to the GLFWwindow
     * \return GLFWwindow
     */
    [[nodiscard]] GLFWwindow *getGlfwWindow() const;

private:
    f64 lastTime_;
    f32 dt_;
    InputState inputState_;
    GLFWwindow *window_;
};

}

#endif // IVY_PLATFORM_H
