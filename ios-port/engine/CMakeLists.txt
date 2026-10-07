# SPDX-License-Identifier: GPL-3.0-or-later
# This target never evaluates Android FTE's top-level CMakeLists.txt.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "NZP iOS engine requires CMAKE_SYSTEM_NAME=iOS and an Apple SDK")
endif()
get_filename_component(NZP_IOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(NZP_STAGE_ROOT "${NZP_IOS_ROOT}/build" CACHE PATH "Prepared isolated sources")
set(NZP_FTE_ROOT "${NZP_STAGE_ROOT}/engine-source")
set(NZP_SDL_ROOT "${NZP_STAGE_ROOT}/SDL2-source")
set(NZP_DEP_ROOT "${NZP_STAGE_ROOT}/dependencies")
if(NOT EXISTS "${NZP_STAGE_ROOT}/fte_sources.cmake")
  message(FATAL_ERROR "Run tools/prepare_engine.py with --android-root before configuring")
endif()
include("${NZP_STAGE_ROOT}/fte_sources.cmake")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static iOS dependencies" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "Static IJG JPEG" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "No dependency tests" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "No dependency tests" FORCE)
set(BUILD_EXECUTABLES OFF CACHE BOOL "No dependency tools" FORCE)
set(BUILD_FRAMEWORK OFF CACHE BOOL "Static codec dependencies" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Static SDL" FORCE)
set(SDL_STATIC ON CACHE BOOL "Static SDL" FORCE)
set(SDL_TEST OFF CACHE BOOL "No SDL tests" FORCE)
set(SDL_TESTS OFF CACHE BOOL "No SDL tests" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "First iOS target uses GLES" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "No raw Bluetooth permission" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "No SDK installation" FORCE)
set(PNG_SHARED OFF CACHE BOOL "Static PNG" FORCE)
set(PNG_STATIC ON CACHE BOOL "Static PNG" FORCE)
set(PNG_FRAMEWORK OFF CACHE BOOL "Static PNG" FORCE)
set(PNG_TESTS OFF CACHE BOOL "No PNG tests" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "No PNG tools" FORCE)
find_package(ZLIB REQUIRED)
add_subdirectory("${NZP_SDL_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/sdl2")
add_subdirectory("${NZP_DEP_ROOT}/libogg" "${CMAKE_CURRENT_BINARY_DIR}/ogg")
set(OGG_LIBRARIES ogg)
set(OGG_INCLUDE_DIRS "${NZP_DEP_ROOT}/libogg/include;${CMAKE_CURRENT_BINARY_DIR}/ogg/include")
add_subdirectory("${NZP_DEP_ROOT}/libvorbis" "${CMAKE_CURRENT_BINARY_DIR}/vorbis")
add_subdirectory("${NZP_DEP_ROOT}/libjpeg" "${CMAKE_CURRENT_BINARY_DIR}/jpeg")
# The upstream static target contains only $<TARGET_OBJECTS:jpeg_objs>.
# Xcode may skip emitting an archive for an object-only consumer (documented
# by CMake's add_library command). Give it one real C source so its Libtool
# phase produces the existing, correctly linked libjpeg.a target product.
set(NZP_JPEG_ANCHOR "${CMAKE_CURRENT_BINARY_DIR}/nzp_jpeg_archive_anchor.c")
file(WRITE "${NZP_JPEG_ANCHOR}"
  "/* Generated Xcode archive anchor; no runtime initialization. */\nvoid NZP_JPEGArchiveAnchor(void) {}\n")
target_sources(jpeg_static PRIVATE "${NZP_JPEG_ANCHOR}")
add_subdirectory("${NZP_DEP_ROOT}/libpng" "${CMAKE_CURRENT_BINARY_DIR}/png")
set(NZP_PLATFORM_SOURCES
  "${NZP_FTE_ROOT}/engine/client/sys_sdl.c"
  "${NZP_FTE_ROOT}/engine/client/in_sdl.c"
  "${NZP_FTE_ROOT}/engine/client/snd_sdl.c"
  "${NZP_FTE_ROOT}/engine/client/cd_null.c"
  "${NZP_FTE_ROOT}/engine/gl/gl_vidsdl.c"
  "${NZP_FTE_ROOT}/engine/common/net_ssl_gnutls.c")
add_library(nzp_fte_engine STATIC ${NZP_FTE_SOURCES} ${NZP_PLATFORM_SOURCES}
  "${NZP_IOS_ROOT}/bridge/nzp_ios_engine.c")
target_include_directories(nzp_fte_engine PRIVATE
  "${NZP_FTE_ROOT}/engine/common" "${NZP_FTE_ROOT}/engine/client"
  "${NZP_FTE_ROOT}/engine/qclib" "${NZP_FTE_ROOT}/engine/gl"
  "${NZP_FTE_ROOT}/engine/server" "${NZP_FTE_ROOT}/engine"
  "${NZP_DEP_ROOT}/libjpeg/libjpeg" "${CMAKE_CURRENT_BINARY_DIR}/jpeg/libjpeg"
  "${NZP_DEP_ROOT}/libpng" "${CMAKE_CURRENT_BINARY_DIR}/png"
  "${NZP_STAGE_ROOT}" "${NZP_IOS_ROOT}/engine" "${NZP_IOS_ROOT}/bridge" "${NZP_IOS_ROOT}/core")
target_compile_definitions(nzp_fte_engine PRIVATE
  NZP_IOS=1 FTE_SDL=1 GLQUAKE=1 MULTITHREAD=1 FTE_LITTLE_ENDIAN
  _SIZEOFPTR=8 CONFIG_FILE_NAME=${NZP_STAGE_ROOT}/config_nzp_ios.h
  LIBJPEG_STATIC LIBPNG_STATIC ZLIB_STATIC LIBVORBISFILE_STATIC
  NO_GNUTLS NO_FREETYPE NO_BOTLIB NO_SPEEX NO_OPUS OMIT_QCC
  SVNREVISION=ios-ce49510f SVNDATE=2026-10-06)
target_compile_options(nzp_fte_engine PRIVATE
  -Wno-pointer-sign -Wno-deprecated-declarations -Wno-unknown-pragmas)
set_target_properties(nzp_fte_engine PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
target_link_libraries(nzp_fte_engine PUBLIC SDL2-static png_static jpeg_static
  vorbisfile vorbis ogg ZLIB::ZLIB "-framework OpenGLES")
# The parent app owns its ObjC/Swift UI, resources and signing settings.
# Compile this file directly into that executable (main cannot live in a dylib).
set(NZP_IOS_ENTRY_SOURCE "${CMAKE_CURRENT_LIST_DIR}/nzp_ios_entry.c" PARENT_SCOPE)
set(NZP_IOS_BRIDGE_INCLUDE_DIR "${NZP_IOS_ROOT}/bridge" PARENT_SCOPE)
