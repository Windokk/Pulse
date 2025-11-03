file(GLOB_RECURSE ENGINE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/glad.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lib/ufbx.c
)

add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine PUBLIC 
  "${CMAKE_SOURCE_DIR}/src/engine/third-party/include/"
  "${CMAKE_SOURCE_DIR}/src/"
)

target_link_directories(Engine PUBLIC
  ${GLFW_LIB_DIR}
  ${FREETYPE_LIB_DIR}
  ${COSEM_PACKER_LIB_DIR}
  ${JOLT_LIB_DIR}
  ${FMOD_LIB_DIR}
)

set_target_properties(Engine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_definitions(Engine PRIVATE BUILD_ENGINE JPH_PROFILE_ENABLED JPH_DEBUG_RENDERER JPH_OBJECT_STREAM)

if(WIN32)
  target_link_libraries(Engine PUBLIC freetype CosemPacker Jolt fmod glfw3dll -lz opengl32 gdi32 user32 shell32 advapi32 pdh)
elseif(UNIX)
  find_package(PNG REQUIRED)
  find_package(BZip2 REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(BROTLI REQUIRED IMPORTED_TARGET libbrotlidec)

  target_link_libraries(Engine PUBLIC freetype CosemPacker Jolt fmod glfw3 -lz PNG::PNG BZip2::BZip2 PkgConfig::BROTLI)
endif()