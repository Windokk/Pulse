file(GLOB_RECURSE GAME_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/platform/glfw/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/glad.c
)

add_library(GameModule SHARED ${GAME_SOURCES})

target_compile_definitions(GameModule PRIVATE BUILD_GAME GLFW_STATIC GLFW_INCLUDE_NONE)

target_include_directories(GameModule PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/include/
  ${CMAKE_SOURCE_DIR}/submodules/glfw/include/
  ${CMAKE_SOURCE_DIR}/submodules/json/single_include/
  ${CMAKE_SOURCE_DIR}/submodules/jolt/
  ${CMAKE_SOURCE_DIR}/submodules/
  ${CMAKE_SOURCE_DIR}/submodules/glm
  ${CMAKE_SOURCE_DIR}/submodules/freetype/include/
  ${FMOD_API_DIR}/inc
)

set(GLFW_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/glfw/src")

target_link_directories(GameModule PUBLIC ${GLFW_LIB_DIR})
target_link_libraries(GameModule PRIVATE Engine glfw3)