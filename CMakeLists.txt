cmake_minimum_required(VERSION 3.21)

project(simple_whitenoise VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Link the MSVC runtime statically so that the plugin does not depend on the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(SWN_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC and Clang only)" OFF)

include(FetchContent)
# CLAP 1.2.10, pinned to the commit of the tag.
FetchContent_Declare(
  clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG 195b42a004144fab0b3cf95e9c067187d15365b7
)
FetchContent_MakeAvailable(clap)

add_library(simple_whitenoise MODULE src/plugin.cpp)
target_link_libraries(simple_whitenoise PRIVATE clap)
target_compile_definitions(simple_whitenoise PRIVATE SWN_VERSION="${PROJECT_VERSION}")
set_target_properties(simple_whitenoise PROPERTIES
  PREFIX ""
  SUFFIX ".clap"
  OUTPUT_NAME "simple-whitenoise"
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

if(MSVC)
  target_compile_options(simple_whitenoise PRIVATE /W4 /permissive- /GS /guard:cf)
  target_link_options(simple_whitenoise PRIVATE /guard:cf /DYNAMICBASE /HIGHENTROPYVA)
else()
  target_compile_options(simple_whitenoise PRIVATE -Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2)
  target_link_options(simple_whitenoise PRIVATE -static-libstdc++ -static-libgcc -Wl,--exclude-libs,ALL -Wl,-z,relro,-z,now)
  if(SWN_SANITIZE)
    target_compile_options(simple_whitenoise PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    target_link_options(simple_whitenoise PRIVATE -fsanitize=address,undefined)
  else()
    # The sanitizer runtimes resolve their symbols at load time, so this check applies only without them.
    target_link_options(simple_whitenoise PRIVATE -Wl,--no-undefined)
  endif()
endif()
