#ifndef IVY_PLATFORM_H
#define IVY_PLATFORM_H

// TODO: hide GLFW
typedef struct GLFWwindow GLFWwindow;

namespace ivy {

class Options;

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

    /**
     * \brief Check whether platform has requested for the application to close
     * \return Whether or not close was requested
     */
    bool isCloseRequested() const;

    /**
     * \brief Get the pointer to the GLFWwindow
     * \return GLFWwindow
     */
    GLFWwindow *getGlfwWindow() const;

private:
    GLFWwindow *window_;
};

}

#endif // IVY_PLATFORM_H
