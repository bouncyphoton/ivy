cmake_minimum_required(VERSION 3.16)
project(ivy)

set(CMAKE_CXX_STANDARD 14)

add_executable(ivy src/main.cpp src/ivy/engine.cpp src/ivy/engine.h src/ivy/types.h src/ivy/graphics/renderer.cpp src/ivy/graphics/renderer.h src/ivy/platform/platform.cpp src/ivy/platform/platform.h src/ivy/log.h src/ivy/utils/utils.cpp src/ivy/utils/utils.h)
target_include_directories(ivy PRIVATE src/)
