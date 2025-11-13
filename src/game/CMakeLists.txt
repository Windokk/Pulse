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
)

set_target_properties(GameModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set(GLFW_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/glfw/src")

if(WIN32)
  target_link_directories(GameModule PUBLIC ${GLFW_LIB_DIR})
  target_link_libraries(GameModule PRIVATE Engine glfw3)
endif()