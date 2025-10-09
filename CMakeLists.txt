cmake_minimum_required(VERSION 3.28.2)

project(Epoch VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COLOR_MAKEFILE ON)

# Set binary dir
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# Platform-specific library paths
if(WIN32)
  set (Qt6_DIR "C:/Qt/6.9.2/mingw_64/lib/cmake/Qt6/") # Change this to your Qt cmake directory
  set(GLFW_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/glfw/glfw-win")
  set(FREETYPE_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/freetype/freetype-win")
  set(COSEM_PACKER_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/filesystem/packer")
  set(JOLT_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/jolt/jolt-win")
  set(FMOD_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/fmod/fmod-win")
elseif(UNIX)
  set (Qt6_DIR "") # Change this to your Qt cmake directory
  set(GLFW_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/glfw/glfw-linux")
  set(FREETYPE_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/freetype/freetype-linux")
  set(COSEM_PACKER_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/filesystem/packer/linux")
  set(JOLT_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/jolt/jolt-linux")
  set(FMOD_LIB_DIR "${CMAKE_SOURCE_DIR}/src/engine/third-party/lib/fmod/fmod-linux")
endif()

# Global definitions
add_definitions(-DGLFW_INCLUDE_NONE -DGLFW_STATIC)

# Add subdirectories
add_subdirectory(src/engine)
add_subdirectory(src/game)
add_subdirectory(src/editor)
add_subdirectory(src/editor-app)