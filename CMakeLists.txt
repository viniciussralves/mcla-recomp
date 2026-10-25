# mcla - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(mcla LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(MCLA_SOURCES
    src/main.cpp
    src/mcla_app.cpp
    src/audio_fallback.cpp
    src/frame_timing.cpp
    src/render_perf.cpp
    src/perf_options.cpp
    src/warp_fix.cpp
    src/button_prompts.cpp
    src/settings_menu.cpp
    src/perf_overlay.cpp
    src/code_pointer_scan.cpp
)

add_executable(mcla ${MCLA_SOURCES})

rexglue_setup_target(mcla)
