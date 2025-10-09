file(GLOB_RECURSE EDITOR_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/platform/qt/*.cpp
)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL)

add_library(EditorModule SHARED ${EDITOR_SOURCES})

set_target_properties(EditorModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_definitions(EditorModule PRIVATE BUILD_EDITOR)

target_include_directories(EditorModule PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)

target_link_libraries(EditorModule PRIVATE Engine Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL)
