cmake_minimum_required(VERSION 3.16)
project(ivy)

set(CMAKE_CXX_STANDARD 17)

add_executable(ivy src/main.cpp src/ivy/engine.cpp src/ivy/engine.h src/ivy/types.h src/ivy/renderer.cpp src/ivy/renderer.h src/ivy/platform/platform.cpp src/ivy/platform/platform.h src/ivy/log.h src/ivy/utils/utils.cpp src/ivy/utils/utils.h src/ivy/graphics/render_context.cpp src/ivy/graphics/render_context.h src/ivy/graphics/vk_utils.cpp src/ivy/graphics/vk_utils.h src/ivy/consts.h)
target_include_directories(ivy PRIVATE src/)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
target_link_libraries(ivy glfw)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(ivy Vulkan::Vulkan)

# Set compile options
if (MSVC)
    target_compile_options(ivy PRIVATE /W4 /WX)
else()
    target_compile_options(ivy PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
