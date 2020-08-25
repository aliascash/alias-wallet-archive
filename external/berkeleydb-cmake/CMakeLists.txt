# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Inspired by The ViaDuck Project for building OpenSSL

cmake_minimum_required(VERSION 3.2)
project(berkeleydb-cmake)

set(BERKELEYDB_ARCHIVE_LOCATION OFF CACHE STRING "Location of BerkeleyDB archive")
set(BERKELEYDB_BUILD_VERSION OFF CACHE STRING "BerkeleyDB version to build")
set(CROSS_ANDROID OFF CACHE BOOL "Cross-compiling for Android?")
set(CROSS OFF CACHE BOOL "Cross-compiling?")
set(CROSS_TARGET OFF CACHE STRING "Cross-compilation target")

# Allow including our modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Mimic system db targets
add_library(bdb INTERFACE)

# Set up fake targets
add_library(bdb_lib STATIC IMPORTED GLOBAL)

target_link_libraries(bdb INTERFACE bdb_lib)

# Set up prefix
set(BERKELEYDB_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "BERKELEYDB_PREFIX: ${BERKELEYDB_PREFIX}")

set(BERKELEYDB_LIBDB_PATH ${BERKELEYDB_PREFIX}/usr/local/BerkeleyDB.${BERKELEYDB_BUILD_VERSION_SHORT}/lib/libdb_cxx.a)
message(STATUS "BERKELEYDB_LIBDB_PATH: ${BERKELEYDB_LIBDB_PATH}")

# Set up BerkeleyDB target
include(BuildBerkeleyDB)

# Set import locations
set_target_properties(bdb_lib PROPERTIES IMPORTED_LOCATION ${BERKELEYDB_LIBDB_PATH})

# Set include locations
target_include_directories(bdb BEFORE INTERFACE ${BERKELEYDB_PREFIX}/usr/local/BerkeleyDB.${BERKELEYDB_BUILD_VERSION_SHORT}/include)

# Add fake targets to common target
add_dependencies(bdb_lib berkeleydb)
