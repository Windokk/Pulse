add_executable(PulseGame
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(CMAKE_BUILD_TYPE Debug)

target_compile_definitions(PulseGame PRIVATE BUILD_ENGINE)

target_link_libraries(PulseGame PRIVATE Engine)

target_include_directories(PulseGame PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)

set_target_properties(PulseGame PROPERTIES
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)