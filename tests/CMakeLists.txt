set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/submodules/googletest ${CMAKE_BINARY_DIR}/submodules/googletest)

file(GLOB_RECURSE PULSE_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(PulseTests ${PULSE_TEST_SOURCES})

target_include_directories(PulseTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PulseTests PRIVATE Engine gtest gtest_main)
target_compile_definitions(PulseTests PRIVATE BUILD_ENGINE)

include(GoogleTest)
gtest_discover_tests(PulseTests)
