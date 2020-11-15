#ifndef IVY_PLATFORM_H
#define IVY_PLATFORM_H

// TODO: hide GLFW
typedef struct GLFWwindow GLFWwindow;

namespace ivy {

class Engine;

/**
 * \brief Platform abstraction layer
 */
class Platform final {
public:
    Platform(Engine &engine);
    ~Platform();

    /**
     * \brief Poll events, update platform specific code
     */
    void update();

    /**
     * \brief Check whether platform has requested for the application to close
     * \return Whether or not close was requested
     */
    bool isCloseRequested() const;

private:
    GLFWwindow *window_;
};

}

#endif // IVY_PLATFORM_H
