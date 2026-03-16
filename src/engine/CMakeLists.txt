file(GLOB_RECURSE ENGINE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/backends/glad/src/gl.c
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/backends/glad/src/vulkan.c
  ${CMAKE_SOURCE_DIR}/submodules/ufbx/ufbx.c
)

add_library(Engine STATIC ${ENGINE_SOURCES})

set(FMOD_API_DIR "" CACHE PATH "Path to fmod api")

set(COMMON_INCLUDE_DIRS 
    "${CMAKE_SOURCE_DIR}/src/"
    "${CMAKE_SOURCE_DIR}/submodules/"
    "${CMAKE_SOURCE_DIR}/submodules/json/single_include/"
    "${CMAKE_SOURCE_DIR}/submodules/jolt/"
    "${CMAKE_SOURCE_DIR}/submodules/glm/"
    "${CMAKE_SOURCE_DIR}/submodules/freetype/include/"
)

# Set the include directories
target_include_directories(Engine PUBLIC
    ${COMMON_INCLUDE_DIRS}
    ${FMOD_API_DIR}/inc
)

# Link directories based on platform (Windows or Unix)
set(FREETYPE_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/freetype")

if(WIN32)
  set(JOLT_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/jolt/Build")
  target_link_directories(Engine PUBLIC
    ${FREETYPE_LIB_DIR}
    ${JOLT_LIB_DIR}
    ${FMOD_API_DIR}/lib/x64
  )
elseif(UNIX)
  set(JOLT_LIB_DIR "${CMAKE_SOURCE_DIR}/build/submodules/jolt/Build/Linux_Debug")
  target_link_directories(Engine PUBLIC
    ${FREETYPE_LIB_DIR}
    ${JOLT_LIB_DIR}
    ${FMOD_API_DIR}/lib/x86_64
  )
endif()

# Define engine-specific compile definitions
target_compile_definitions(Engine PRIVATE BUILD_ENGINE)

# Link the necessary libraries based on the platform
if(WIN32)
  target_link_libraries(Engine PRIVATE freetype Jolt fmod -lz opengl32 gdi32 user32 shell32 advapi32 pdh)
elseif(UNIX)
  target_link_libraries(Engine PRIVATE freetype Jolt fmod -lz)
endif()