add_executable(EpochEditor
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_compile_definitions(EpochEditor PRIVATE BUILD_ENGINE)

target_link_libraries(EpochEditor PRIVATE Engine)

target_include_directories(EpochEditor PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)

set_target_properties(EpochEditor PROPERTIES
    ${BUNDLE_ID_OPTION}
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)