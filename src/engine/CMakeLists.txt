file(GLOB_RECURSE ENGINE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/glad.c
  ${CMAKE_SOURCE_DIR}/submodules/ufbx/ufbx.c
)

add_library(Engine STATIC ${ENGINE_SOURCES})

set(FMOD_API_DIR "" CACHE PATH "Path to fmod api")

set(CPP_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/misc/
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/rendering/
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/physics/
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/components/audio/
)

set(COMMON_INCLUDE_DIRS 
    "${CMAKE_SOURCE_DIR}/src/"
    "${CMAKE_SOURCE_DIR}/submodules/"
    "${CMAKE_SOURCE_DIR}/submodules/json/single_include/"
    "${CMAKE_SOURCE_DIR}/submodules/jolt/"
    "${CMAKE_SOURCE_DIR}/submodules/glm/"
    "${CMAKE_SOURCE_DIR}/submodules/freetype/include/"
)

set(DIR_FLAGS "")
foreach(DIR ${CPP_INCLUDE_DIRS})
    list(APPEND DIR_FLAGS "--dir" ${DIR})
endforeach()

set(INCLUDE_FLAGS "")
foreach(DIR ${COMMON_INCLUDE_DIRS})
    list(APPEND INCLUDE_FLAGS "-I" ${DIR})
endforeach()

list(APPEND INCLUDE_FLAGS "-I" ${FMOD_API_DIR}/inc)

set(CLANG_DIR "" CACHE PATH "Path to clang lib")
set(CPP_INC_DIR "" CACHE PATH "Path of cpp include dir")

if(UNIX)
  add_custom_command(
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      TARGET Engine
      PRE_LINK
      USES_TERMINAL
      COMMAND ./PulseReflect
              --clang ${CLANG_DIR}
              --cpp ${CPP_INC_DIR}
              ${DIR_FLAGS}
              ${INCLUDE_FLAGS}
  )
elseif(WIN32)
  add_custom_command(
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/
      TARGET Engine
      PRE_LINK
      USES_TERMINAL
      COMMAND ./PulseReflect
              --clang ${CLANG_DIR}
              --cpp ${CPP_INC_DIR}
              ${DIR_FLAGS}
              ${INCLUDE_FLAGS}
  )
endif()

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