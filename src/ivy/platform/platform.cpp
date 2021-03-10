#include "platform.h"
#include "ivy/log.h"
#include "ivy/engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static void key_callback(GLFWwindow *window, int key, [[maybe_unused]] int scancode,
                         int action, [[maybe_unused]] int mods) {
    ivy::InputState *input = static_cast<ivy::InputState *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        input->pressKey(key);
    } else if (action == GLFW_RELEASE) {
        input->releaseKey(key);
    }
}

namespace ivy {

Platform::Platform(const Options &options) {
    LOG_CHECKPOINT();

    if (!glfwInit()) {
        Log::fatal("Failed to init glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(options.renderWidth, options.renderHeight, options.appName, nullptr, nullptr);

    if (!window_) {
        Log::fatal("Failed to create window");
    }

    if (!glfwVulkanSupported()) {
        Log::fatal("Vulkan is not supported");
    }

    glfwSetWindowUserPointer(window_, &inputState_);
    glfwSetKeyCallback(window_, key_callback);
}

Platform::~Platform() {
    LOG_CHECKPOINT();
    glfwTerminate();
}

void Platform::update() {
    inputState_.transitionKeyStates();

    glfwPollEvents();

    f64 now = glfwGetTime();
    dt_ = static_cast<f32>(now - lastTime_);
    lastTime_ = now;
}

bool Platform::isCloseRequested() const {
    return glfwWindowShouldClose(window_);
}

GLFWwindow *Platform::getGlfwWindow() const {
    return window_;
}

}
