cmake_minimum_required(VERSION 3.16)
project(ivy)

set(CMAKE_CXX_STANDARD 17)

add_executable(ivy src/main.cpp src/ivy/engine.cpp src/ivy/engine.h src/ivy/types.h src/test_game/renderer.cpp src/test_game/renderer.h src/ivy/platform/platform.cpp src/ivy/platform/platform.h src/ivy/log.h src/ivy/utils/utils.cpp src/ivy/utils/utils.h src/ivy/graphics/render_device.cpp src/ivy/graphics/render_device.h src/ivy/graphics/vk_utils.cpp src/ivy/graphics/vk_utils.h src/ivy/consts.h src/ivy/graphics/command_buffer.cpp src/ivy/graphics/command_buffer.h src/ivy/options.h src/ivy/graphics/framebuffer.h src/ivy/graphics/vertex.h src/ivy/graphics/graphics_pass.cpp src/ivy/graphics/graphics_pass.h src/ivy/graphics/shader.h src/ivy/graphics/vertex_description.h src/ivy/graphics/descriptor_set.cpp src/ivy/graphics/descriptor_set.h src/ivy/graphics/descriptor_set_cache.cpp src/ivy/graphics/descriptor_set_cache.h src/ivy/graphics/geometry.cpp src/ivy/graphics/geometry.h src/ivy/scene/entity.inl src/ivy/scene/entity.h src/ivy/scene/components/transform.h src/ivy/scene/components/model.h src/ivy/scene/components/component.h src/ivy/resources/resource_manager.cpp src/ivy/resources/resource_manager.h src/ivy/platform/input_state.cpp src/ivy/platform/input_state.h src/test_game/test_game.cpp src/test_game/test_game.h src/ivy/graphics/mesh.h src/ivy/graphics/material.h src/ivy/resources/resource.h src/ivy/resources/model_resource.h src/ivy/resources/texture_resource.h src/ivy/scene/scene.cpp src/ivy/scene/scene.h src/ivy/scene/scene.inl src/ivy/graphics/texture.cpp src/ivy/graphics/texture.h)
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

# Vulkan Memory Allocator (VMA)
# Include as system to ignore warnings
SET(VMA_SRC ${PROJECT_SOURCE_DIR}/external/VulkanMemoryAllocator/src)
include_directories(SYSTEM ${VMA_SRC})

# GLM
target_include_directories(ivy PRIVATE external/glm)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SILENT_WARNINGS)

# Assimp
set(BUILD_SHARED_LIBS off)
set(ASSIMP_NO_EXPORT on)
add_subdirectory(external/assimp)
target_link_libraries(ivy assimp)

# stb
# Include as system to ignore warnings
include_directories(SYSTEM external/stb)

# Set compile options
if (MSVC)
    # Treat warnings as errors
    target_compile_options(ivy PRIVATE /W4 /WX)

    target_compile_options(ivy PRIVATE /experimental:external /external:W0 /external:I ${VMA_SRC})
else()
    # Treat warnings as errors
    target_compile_options(ivy PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
