# hpm — HolePunchMan
# KaisarCode — CMake build configuration
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.14)
project(hpm LANGUAGES C)
set(CMAKE_C_STANDARD 11)

if(NOT DEFINED HPM_BUILD_VERSION)
    set(HPM_BUILD_VERSION 0)
endif()

if(NOT MSVC)
    add_compile_options(-O3 -Wall -Wextra -Werror)
    add_definitions(-D_GNU_SOURCE)
else()
    add_compile_options(/O2 /W3 /WX)
endif()

# Shared library (for external use)
set(HPM_SOURCES
    src/libhpm.c
    src/version.c
    lib/monocypher/monocypher.c
)

set_source_files_properties(src/version.c PROPERTIES
    COMPILE_DEFINITIONS KC_HPM_BUILD_VERSION=${HPM_BUILD_VERSION}
)

add_library(hpm_shared SHARED ${HPM_SOURCES})
set_target_properties(hpm_shared PROPERTIES OUTPUT_NAME "hpm")
target_include_directories(hpm_shared PRIVATE src lib/monocypher)

# Static library (for external use)
add_library(hpm_static STATIC ${HPM_SOURCES})
set_target_properties(hpm_static PROPERTIES OUTPUT_NAME "hpm")
target_include_directories(hpm_static PRIVATE src lib/monocypher)

# Binary: hpm
add_executable(hpm src/hpm.c ${HPM_SOURCES})
target_include_directories(hpm PRIVATE src lib/monocypher)

if(UNIX AND NOT ANDROID)
    target_link_libraries(hpm_shared pthread m)
    target_link_libraries(hpm_static pthread m)
    target_link_libraries(hpm pthread m)
elseif(MINGW)
    target_link_libraries(hpm_shared ws2_32 bcrypt)
    target_link_libraries(hpm_static ws2_32 bcrypt)
    target_link_libraries(hpm ws2_32 bcrypt)
elseif(MSVC)
    target_link_libraries(hpm_shared bcrypt)
    target_link_libraries(hpm_static bcrypt)
    target_link_libraries(hpm bcrypt)
endif()
