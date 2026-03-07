file(GLOB_RECURSE EDITOR_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/glad.c
  ${CMAKE_SOURCE_DIR}/submodules/imguizmo/ImGuizmo.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/imgui_demo.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_SOURCE_DIR}/submodules/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(PulseEditor ${EDITOR_SOURCES})

target_compile_definitions(PulseEditor PRIVATE BUILD_ENGINE)

target_include_directories(PulseEditor PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/include/
  ${CMAKE_SOURCE_DIR}/submodules/imgui/
  ${CMAKE_SOURCE_DIR}/submodules/imguizmo/
  ${CMAKE_SOURCE_DIR}/submodules/glfw/include/
)

set_target_properties(PulseEditor PROPERTIES
    ${BUNDLE_ID_OPTION}
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set(GLFW_LIB_DIR ${CMAKE_BINARY_DIR})

target_link_directories(PulseEditor PUBLIC ${GLFW_LIB_DIR})

target_link_libraries(PulseEditor PRIVATE Engine glfw3)