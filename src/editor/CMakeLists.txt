file(GLOB_RECURSE EDITOR_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/default.qrc
  ${CMAKE_SOURCE_DIR}/submodules/imguizmo/ImGuizmo.cpp
)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL Svg)

add_library(EditorModule SHARED ${EDITOR_SOURCES})

set(QT_ADS_PATH ${CMAKE_BINARY_DIR}/x64/lib CACHE PATH "Path to qt ads lib")

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

if(WIN32)
  set(QT_ADS_NAME qtadvanceddocking)
elseif(UNIX)
  set(QT_ADS_NAME qtadvanceddocking-qt6d)
endif()


target_link_libraries(EditorModule PRIVATE ${QT_ADS_NAME} qt_imgui_widgets imgui Engine Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL Qt6::Svg)