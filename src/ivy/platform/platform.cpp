#include "platform.h"
#include "ivy/log.h"
#include "ivy/engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
}

Platform::~Platform() {
    LOG_CHECKPOINT();
    glfwTerminate();
}

void Platform::update() {
    glfwPollEvents();
}

bool Platform::isCloseRequested() const {
    return glfwWindowShouldClose(window_);
}

GLFWwindow *Platform::getGlfwWindow() const {
    return window_;
}

}
