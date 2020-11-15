cmake_minimum_required(VERSION 3.16)
project(ivy)

set(CMAKE_CXX_STANDARD 14)

add_executable(ivy src/main.cpp src/ivy/engine.cpp src/ivy/engine.h src/ivy/types.h src/ivy/graphics/renderer.cpp src/ivy/graphics/renderer.h src/ivy/platform/platform.cpp src/ivy/platform/platform.h src/ivy/log.h src/ivy/utils/utils.cpp src/ivy/utils/utils.h)
target_include_directories(ivy PRIVATE src/)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
target_link_libraries(ivy glfw)
