# MIT License
#
# Copyright (c) 2019 The Spectrecoin Team
#
# Inspired by The ViaDuck Project for building OpenSSL
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

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
message(STATUS "BERKELEYDB_LIBDB_PATH ${BERKELEYDB_LIBDB_PATH}")

set(BERKELEYDB_INCLUDE_PATH ${BERKELEYDB_PREFIX}/usr/local/BerkeleyDB.${BERKELEYDB_BUILD_VERSION_SHORT}/include PARENT_SCOPE)

# Set up BerkeleyDB target
include(BuildBerkeleyDB)

# Set import locations
set_target_properties(bdb_lib PROPERTIES IMPORTED_LOCATION ${BERKELEYDB_LIBDB_PATH})

# Set include locations
target_include_directories(bdb BEFORE INTERFACE ${BERKELEYDB_PREFIX}/usr/local/BerkeleyDB.${BERKELEYDB_BUILD_VERSION_SHORT}/include)

# Add fake targets to common target
add_dependencies(bdb_lib berkeleydb)
