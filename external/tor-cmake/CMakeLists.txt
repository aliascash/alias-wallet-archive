# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Inspired by The ViaDuck Project for building OpenSSL

cmake_minimum_required(VERSION 3.2)
project(tor-cmake)

set(TOR_ARCHIVE_LOCATION OFF CACHE STRING "Location of Tor archive")
set(TOR_BUILD_VERSION OFF CACHE STRING "Tor version to build")
set(CROSS_ANDROID OFF CACHE BOOL "Cross-compiling for Android?")
set(CROSS OFF CACHE BOOL "Cross-compiling?")
set(CROSS_TARGET OFF CACHE STRING "Cross-compilation target")

# Allow including our modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Mimic system db targets
add_library(libtor INTERFACE)

# Set up fake targets
add_library(lib_tor STATIC IMPORTED GLOBAL)

target_link_libraries(libtor INTERFACE lib_tor)

# Set up prefix
set(TOR_LIBTOR_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "TOR_LIBTOR_PREFIX: ${TOR_LIBTOR_PREFIX}")

set(TOR_LIBTOR_PATH ${TOR_LIBTOR_PREFIX}/usr/local/bin/tor)
message(STATUS "TOR_LIBTOR_PATH ${TOR_LIBTOR_PATH}")

# Set up Tor target
include(BuildTor)

# Set import locations
set_target_properties(lib_tor PROPERTIES IMPORTED_LOCATION ${TOR_LIBTOR_PATH})

# Set include locations
target_include_directories(libtor BEFORE INTERFACE ${TOR_LIBTOR_PREFIX}/usr/local/include)

#set(TOR_INCLUDE_PATH
#        ${TOR_LIBTOR_PREFIX}/libtorExternal-prefix/src/libtorExternal/
#        ${TOR_LIBTOR_PREFIX}/libtorExternal-prefix/src/libtorExternal/src/app/main/
#        ${TOR_LIBTOR_PREFIX}/libtorExternal-prefix/src/libtorExternal/src/feature/api/
#        PARENT_SCOPE
#        )

# Add fake targets to common target
add_dependencies(lib_tor libtorExternal)
