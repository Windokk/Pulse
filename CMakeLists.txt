cmake_minimum_required(VERSION 3.28.2)

project(Pulse VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_BUILD_TYPE Debug)

# Set binary dir
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

set(FMOD_API_DIR "" CACHE PATH "Path to fmod api")

configure_file(${CMAKE_SOURCE_DIR}/src/config.h.in config.h)

include_directories(${CMAKE_BINARY_DIR})

# Subdirs (Third party)

add_subdirectory(submodules/freetype)
add_subdirectory(submodules/glfw)
add_subdirectory(submodules/glm)
add_subdirectory(submodules/jolt/Build)
#add_subdirectory(submodules/qtads) For now it seems that the cmake build is broken --> use Qmake instead
add_subdirectory(submodules/qtimgui)

# Subdirs (Pulse)
add_subdirectory(src/engine)
add_subdirectory(src/game)
add_subdirectory(src/editor)
add_subdirectory(src/game-app)
add_subdirectory(src/editor-app)