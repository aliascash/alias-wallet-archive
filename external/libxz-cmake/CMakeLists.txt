# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Inspired by The ViaDuck Project for building OpenSSL

cmake_minimum_required(VERSION 3.2)
project(libxz-cmake)

set(LIBLZMA_ARCHIVE_LOCATION OFF CACHE STRING "Location of XZLib archive")
set(LIBLZMA_BUILD_VERSION OFF CACHE STRING "XZLib version to build")
set(CROSS_ANDROID OFF CACHE BOOL "Cross-compiling for Android?")
set(CROSS OFF CACHE BOOL "Cross-compiling?")
set(CROSS_TARGET OFF CACHE STRING "Cross-compilation target")

# Allow including our modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Mimic system db targets
add_library(liblzma INTERFACE)

# Set up fake targets
add_library(lib_lzma STATIC IMPORTED GLOBAL)

target_link_libraries(liblzma INTERFACE lib_lzma)

# Set up prefix
set(LIBLZMA_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "LIBLZMA_PREFIX: ${LIBLZMA_PREFIX}")

set(LIBLZMA_PATH ${LIBLZMA_PREFIX}/usr/local/lib/liblzma.a)
message(STATUS "LIBLZMA_PATH ${LIBLZMA_PATH}")

# Set up XZLib target
include(BuildXZLib)

# Set import locations
set_target_properties(lib_lzma PROPERTIES IMPORTED_LOCATION ${LIBLZMA_PATH})

# Set include locations
target_include_directories(liblzma BEFORE INTERFACE ${LIBLZMA_PREFIX}/usr/local/include)

# Add fake targets to common target
add_dependencies(lib_lzma libxzExternal)
