find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL)

qt_add_executable(Epoch
    MANUAL_FINALIZATION
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_compile_definitions(Epoch PRIVATE BUILD_ENGINE)

target_link_libraries(Epoch PRIVATE Engine Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL)

target_include_directories(Epoch PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)

set_target_properties(Epoch PROPERTIES
    ${BUNDLE_ID_OPTION}
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

qt_finalize_executable(Epoch)