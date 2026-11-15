cmake_minimum_required(VERSION 3.21)

project(simple_whitenoise VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Link the MSVC runtime statically so that the plugin does not depend on the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# The static libraries below are linked into the plugin modules. Only the symbols marked for export,
# such as clap_entry, GetPluginFactory, and the VST 3 module entry points, stay visible.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

option(SWN_SANITIZE "Build the CLAP plugin with AddressSanitizer and UndefinedBehaviorSanitizer (GCC and Clang only)" OFF)

# Hardening applies to every compiled source, including the SDKs that FetchContent adds below.
if(MSVC)
  add_compile_options(/GS /guard:cf)
else()
  add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
endif()

include(FetchContent)
# CLAP 1.2.10, pinned to the commit of the tag. FetchContent only downloads it: the SOURCE_SUBDIR has no
# CMakeLists.txt. clap-wrapper adds it through CLAP_SDK_ROOT with EXCLUDE_FROM_ALL, so its install rules
# stay out of the release layout and clap-wrapper does not download a second copy.
FetchContent_Declare(
  clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG 195b42a004144fab0b3cf95e9c067187d15365b7
  SOURCE_SUBDIR download-only
)
FetchContent_MakeAvailable(clap)
set(CLAP_SDK_ROOT "${clap_SOURCE_DIR}")
# clap-wrapper v0.17.0, pinned to the commit of the tag. It downloads the VST 3 SDK at the version it pins.
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "" FORCE)
set(CLAP_WRAPPER_BUILD_AUV2 FALSE CACHE BOOL "" FORCE)
FetchContent_Declare(
  clap-wrapper
  GIT_REPOSITORY https://github.com/free-audio/clap-wrapper.git
  GIT_TAG 806bba0a5f792e3966b99c1561ab9952d3316d7c
)
FetchContent_MakeAvailable(clap-wrapper)

# The file names stay those of 1.0.x: simple-whitenoise.clap, and simple-whitenoise.vst3 next to it.
set(PLUGIN_FILE_NAME "simple-whitenoise")

add_library(simple-whitenoise-impl STATIC src/plugin.cpp)
target_include_directories(simple-whitenoise-impl PUBLIC src)
# clapwrapper/vst3.h, for the VST 3 factory information returned in src/plugin.cpp.
target_include_directories(simple-whitenoise-impl PRIVATE "${clap-wrapper_SOURCE_DIR}/include")
target_compile_definitions(simple-whitenoise-impl PRIVATE SWN_VERSION="${PROJECT_VERSION}")
target_link_libraries(simple-whitenoise-impl PUBLIC clap)

# clap-wrapper links the VST 3 module with --no-undefined, which the sanitizer runtimes do not satisfy.
if(SWN_SANITIZE)
  set(PLUGIN_FORMATS CLAP)
else()
  set(PLUGIN_FORMATS CLAP VST3)
endif()

make_clapfirst_plugins(
  TARGET_NAME simple-whitenoise
  IMPL_TARGET simple-whitenoise-impl
  OUTPUT_NAME "${PLUGIN_FILE_NAME}"
  ENTRY_SOURCE src/plugin-entry.cpp
  BUNDLE_IDENTIFIER "jp.allegromoltov.simple-whitenoise"
  BUNDLE_VERSION ${PROJECT_VERSION}
  COPY_AFTER_BUILD FALSE
  # VST 3 hosts and the registry both expect the bundle folder layout on Windows.
  WINDOWS_FOLDER_VST3 TRUE
  PLUGIN_FORMATS ${PLUGIN_FORMATS}
  ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/assets
)

if(MSVC)
  target_compile_options(simple-whitenoise-impl PRIVATE /W4 /permissive-)
else()
  target_compile_options(simple-whitenoise-impl PRIVATE -Wall -Wextra)
endif()

foreach(module IN ITEMS simple-whitenoise_clap simple-whitenoise_vst3)
  if(NOT TARGET ${module})
    continue()
  endif()
  if(MSVC)
    target_link_options(${module} PRIVATE /guard:cf /DYNAMICBASE /HIGHENTROPYVA)
  else()
    # The statically linked C++ runtime stays hidden. --exclude-libs,ALL would also hide the VST 3 module
    # entry points ModuleEntry and ModuleExit, which the VST 3 SDK defines in a static library.
    target_link_options(${module} PRIVATE -static-libstdc++ -static-libgcc
                        -Wl,--exclude-libs,libstdc++.a:libgcc.a:libgcc_eh.a -Wl,-z,relro,-z,now)
    if(NOT SWN_SANITIZE)
      # The sanitizer runtimes resolve their symbols at load time, so this check applies only without them.
      target_link_options(${module} PRIVATE -Wl,--no-undefined)
    endif()
  endif()
endforeach()

if(SWN_SANITIZE AND NOT MSVC)
  set(sanitize_flags -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
  target_compile_options(simple-whitenoise-impl PRIVATE ${sanitize_flags})
  target_compile_options(simple-whitenoise_clap PRIVATE ${sanitize_flags})
  target_link_options(simple-whitenoise_clap PRIVATE -fsanitize=address,undefined)
endif()
if(SWN_SANITIZE)
  # A sanitizer build is for testing only and has no release layout.
  return()
endif()

# Release layout: `cmake --install build --config Release --prefix <dir>`, then zip the contents of <dir>.
# The archive carries the license texts of everything linked into the plugins.
# clap-wrapper downloads the VST 3 SDK to this place; a layout change in a new clap-wrapper fails here.
set(VST3_SDK_LICENSE "${CMAKE_BINARY_DIR}/cpm/vst3sdk/LICENSE.txt")
set(CLAP_SDK_LICENSE "${clap_SOURCE_DIR}/LICENSE")
foreach(license IN ITEMS "${VST3_SDK_LICENSE}" "${CLAP_SDK_LICENSE}")
  if(NOT EXISTS "${license}")
    message(FATAL_ERROR "License file not found: ${license}")
  endif()
endforeach()
# fmt keeps its license in the leading comment of format.h.
file(READ "${clap-wrapper_SOURCE_DIR}/libs/fmt/fmt/format.h" FMT_HEADER)
string(FIND "${FMT_HEADER}" "*/" FMT_COMMENT_END)
string(SUBSTRING "${FMT_HEADER}" 0 ${FMT_COMMENT_END} FMT_LICENSE_TEXT)
string(REGEX REPLACE "^/\\*\n" "" FMT_LICENSE_TEXT "${FMT_LICENSE_TEXT}")
file(WRITE "${CMAKE_BINARY_DIR}/fmt-LICENSE.txt" "${FMT_LICENSE_TEXT}")

# clap-wrapper puts the Windows plugins in per-format folders and the Linux plugins directly in the asset folder.
if(WIN32)
  set(VST3_BUNDLE "${CMAKE_BINARY_DIR}/assets/VST3/${PLUGIN_FILE_NAME}.vst3")
else()
  set(VST3_BUNDLE "${CMAKE_BINARY_DIR}/assets/${PLUGIN_FILE_NAME}.vst3")
endif()
install(FILES "$<TARGET_FILE:simple-whitenoise_clap>" DESTINATION .)
install(DIRECTORY "${VST3_BUNDLE}" DESTINATION .)
install(FILES LICENSE packaging/README.txt DESTINATION .)
install(FILES "${VST3_SDK_LICENSE}" DESTINATION THIRD_PARTY_LICENSES RENAME vst3sdk.txt)
install(FILES "${CLAP_SDK_LICENSE}" DESTINATION THIRD_PARTY_LICENSES RENAME clap.txt)
install(FILES "${clap-wrapper_SOURCE_DIR}/LICENSE" DESTINATION THIRD_PARTY_LICENSES RENAME clap-wrapper.txt)
install(FILES "${CMAKE_BINARY_DIR}/fmt-LICENSE.txt" DESTINATION THIRD_PARTY_LICENSES RENAME fmt.txt)
