cmake_minimum_required(VERSION 3.22)
project(NZPMobileIOS VERSION 0.1.0 LANGUAGES C CXX OBJC)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "This target requires the Apple iOS SDK. Run tests/run_tests.sh for portable host tests.")
endif()
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(NZP_ANDROID_SOURCE "" CACHE PATH "Root of the corresponding v4 Android source export")
if(NOT EXISTS "${NZP_ANDROID_SOURCE}/app/src/main/assets/content-manifest.sha256")
  message(FATAL_ERROR "Set NZP_ANDROID_SOURCE to the verified Android source export or repository root.")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_content.py" "${NZP_ANDROID_SOURCE}"
                RESULT_VARIABLE NZP_VERIFY_RESULT)
if(NOT NZP_VERIFY_RESULT EQUAL 0)
  message(FATAL_ERROR "Pinned game content verification failed")
endif()

add_library(nzp_mobile_core STATIC core/nzp_mobile_state.c core/nzp_touch.c layout/nzp_hud_layout.c)
target_include_directories(nzp_mobile_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/core" "${CMAKE_CURRENT_SOURCE_DIR}/layout")
add_subdirectory(engine)

file(GLOB_RECURSE NZP_GAME_FILES CONFIGURE_DEPENDS "${NZP_ANDROID_SOURCE}/app/src/main/assets/game/*")
foreach(file IN LISTS NZP_GAME_FILES)
  if(NOT IS_DIRECTORY "${file}")
    file(RELATIVE_PATH relative "${NZP_ANDROID_SOURCE}/app/src/main/assets/game" "${file}")
    get_filename_component(folder "${relative}" DIRECTORY)
    set_source_files_properties("${file}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/game/${folder}")
  endif()
endforeach()
set(NZP_NOTICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/notices/NOTICE-iOS.txt"
  "${CMAKE_CURRENT_SOURCE_DIR}/notices/SDL2-zlib.txt")
foreach(notice GPL-3.0.txt NZP-asset-attributions.txt NZP-QuakeC-GPL-2.0.txt
    NZP-assets-CC-BY-SA-4.0.txt FTE-engine-license.txt JPEG-README.txt
    PNG-license.txt Ogg-BSD.txt Vorbis-BSD.txt)
  list(APPEND NZP_NOTICE_FILES "${NZP_ANDROID_SOURCE}/app/src/main/assets/notices/${notice}")
endforeach()
set_source_files_properties(${NZP_NOTICE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/notices")
set(NZP_HUD_ICONS platform/apple/assets/double-points.png platform/apple/assets/insta-kill.png)
set_source_files_properties(${NZP_HUD_ICONS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
add_executable(NZPMobile MACOSX_BUNDLE
  platform/apple/NZPTouchOverlay.m
  ${NZP_IOS_ENTRY_SOURCE}
  ${NZP_GAME_FILES} ${NZP_NOTICE_FILES} ${NZP_HUD_ICONS})
target_include_directories(NZPMobile PRIVATE bridge core layout platform/apple)
target_compile_options(NZPMobile PRIVATE "$<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>")
target_link_libraries(NZPMobile PRIVATE nzp_fte_engine nzp_mobile_core "-framework UIKit" "-framework Foundation")
set_target_properties(NZPMobile PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platform/apple/Info.plist.in"
  MACOSX_BUNDLE_GUI_IDENTIFIER org.nzp.mobile.ios.preview
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER org.nzp.mobile.ios.preview
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
  XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
  XCODE_ATTRIBUTE_ENABLE_BITCODE NO
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_ENABLE_USER_SCRIPT_SANDBOXING YES)
