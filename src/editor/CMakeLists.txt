file(GLOB_RECURSE EDITOR_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/platform/qt/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/default.qrc
  ${CMAKE_SOURCE_DIR}/submodules/imguizmo/ImGuizmo.cpp
)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL)

add_library(EditorModule SHARED ${EDITOR_SOURCES})

set_target_properties(EditorModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set(QT_ADS_PATH ${CMAKE_SOURCE_DIR}/submodules/qtads/build/Desktop_Qt_6_9_2_MinGW_64_bit-Debug/lib/ CACHE PATH "Path to qt ads lib")

target_compile_definitions(EditorModule PRIVATE BUILD_EDITOR)

target_include_directories(EditorModule PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
  ${CMAKE_SOURCE_DIR}/submodules/qtads/src/
  ${CMAKE_SOURCE_DIR}/submodules/qtimgui/src/
  ${CMAKE_SOURCE_DIR}/submodules/qtimgui/modules/
  ${CMAKE_SOURCE_DIR}/submodules/imguizmo/
)

target_link_directories(EditorModule PRIVATE 
  ${CMAKE_SOURCE_DIR}/build/submodules/qtimgui/src/
  ${CMAKE_SOURCE_DIR}/build/submodules/qtimgui/modules/
  ${QT_ADS_PATH}
)

target_link_libraries(EditorModule PRIVATE qtadvanceddocking qt_imgui_widgets imgui Engine Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL)