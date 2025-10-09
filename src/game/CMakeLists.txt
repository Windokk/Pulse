file(GLOB_RECURSE GAME_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/platform/glfw/*.cpp
)

add_library(GameModule SHARED ${GAME_SOURCES})

target_compile_definitions(GameModule PRIVATE BUILD_GAME)

target_include_directories(GameModule PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)

set_target_properties(GameModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
  target_link_directories(GameModule PUBLIC ${GLFW_LIB_DIR})
  target_link_libraries(GameModule PRIVATE Engine glfw3dll)
elseif(UNIX)
  target_link_libraries(GameModule PRIVATE Engine)
endif()