add_executable(PulseEditor
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_compile_definitions(PulseEditor PRIVATE BUILD_ENGINE)

target_link_libraries(PulseEditor PRIVATE Engine)

target_include_directories(PulseEditor PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)

set_target_properties(PulseEditor PROPERTIES
    ${BUNDLE_ID_OPTION}
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)