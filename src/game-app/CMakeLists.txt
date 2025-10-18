add_executable(EpochGame
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(CMAKE_BUILD_TYPE Debug)

target_compile_definitions(EpochGame PRIVATE BUILD_ENGINE)

target_link_libraries(EpochGame PRIVATE Engine)

target_include_directories(EpochGame PRIVATE
  ${CMAKE_SOURCE_DIR}/src/
)


set_target_properties(EpochGame PROPERTIES
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)