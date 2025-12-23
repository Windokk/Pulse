file(GLOB_RECURSE ENGINE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/glad.c
  ${CMAKE_SOURCE_DIR}/submodules/ufbx/ufbx.c
)

add_library(Engine STATIC ${ENGINE_SOURCES})

set(FMOD_API_DIR "" CACHE PATH "Path to fmod api")

# Define the working directories and the command parameters
set(CPP_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/misc/
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/rendering/
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/physics/
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/audio/
)

# COMMON_INCLUDE_DIRS is a semicolon-separated list of directories
set(COMMON_INCLUDE_DIRS 
    "/home/windokk/Documents/GitHub/Pulse/src/"
    "/home/windokk/Documents/GitHub/Pulse/submodules/"
    "/home/windokk/Documents/GitHub/Pulse/submodules/json/single_include/"
    "/home/windokk/Documents/GitHub/Pulse/submodules/jolt/"
    "/home/windokk/Documents/GitHub/Pulse/submodules/glm/"
    "/home/windokk/Documents/GitHub/Pulse/submodules/freetype/include/"
)

# Prepare the --dir flags for each directory in CPP_INCLUDE_DIRS
set(DIR_FLAGS "")
foreach(DIR ${CPP_INCLUDE_DIRS})
    list(APPEND DIR_FLAGS "--dir" ${DIR})  # Add each directory as a separate --dir argument
endforeach()

# Prepare the -I flags for each directory in COMMON_INCLUDE_DIRS
set(INCLUDE_FLAGS "")
foreach(DIR ${COMMON_INCLUDE_DIRS})
    list(APPEND INCLUDE_FLAGS "-I" ${DIR})  # Add each directory as a separate -I argument
endforeach()

# Prepare the -I flags for FMOD API directory
list(APPEND INCLUDE_FLAGS "-I" ${FMOD_API_DIR}/inc)

# Print the final command for debugging
message(STATUS "Generated PulseReflect command: ./PulseReflect --clang /usr/lib/clang/21 --cpp /usr/include/c++/15.2.1 ${DIR_FLAGS} ${INCLUDE_FLAGS}")

# Now add the flags to your custom command
add_custom_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    TARGET Engine
    PRE_LINK
    USES_TERMINAL
    COMMAND ./PulseReflect
            --clang /usr/lib/clang/21
            --cpp /usr/include/c++/15.2.1
            ${DIR_FLAGS}  # Add each directory as a separate --dir argument
            ${INCLUDE_FLAGS}  # Add each directory as a separate -I argument
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