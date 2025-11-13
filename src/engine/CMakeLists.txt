file(GLOB_RECURSE ENGINE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/glad.c
  ${CMAKE_SOURCE_DIR}/submodules/ufbx/ufbx.c
)

add_library(Engine STATIC ${ENGINE_SOURCES})

set(FMOD_API_DIR "" CACHE PATH "Path to fmod api")

target_include_directories(Engine PUBLIC 
  "${CMAKE_SOURCE_DIR}/src/engine/third-party/include/"
  "${CMAKE_SOURCE_DIR}/src/"
  "${CMAKE_SOURCE_DIR}/submodules/json/single_include/"
  "${CMAKE_SOURCE_DIR}/submodules/jolt/"
  "${CMAKE_SOURCE_DIR}/submodules/"
  "${CMAKE_SOURCE_DIR}/submodules/glm"
  "${CMAKE_SOURCE_DIR}/submodules/freetype/include/"
  ${FMOD_API_DIR}/inc
)

set(FREETYPE_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/freetype")
set(JOLT_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/jolt/Build")

target_link_directories(Engine PUBLIC
  ${FREETYPE_LIB_DIR}
  ${JOLT_LIB_DIR}
  ${FMOD_API_DIR}/lib/x64
)

set_target_properties(Engine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_definitions(Engine PRIVATE BUILD_ENGINE)

if(WIN32)
  target_link_libraries(Engine PRIVATE freetype Jolt fmod -lz opengl32 gdi32 user32 shell32 advapi32 pdh)
endif()