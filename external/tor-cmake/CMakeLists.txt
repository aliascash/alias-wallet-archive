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
project(tor-cmake)

set(TOR_ARCHIVE_LOCATION OFF CACHE STRING "Location of Tor archive")
set(TOR_BUILD_VERSION OFF CACHE STRING "Tor version to build")
set(CROSS_ANDROID OFF CACHE BOOL "Cross-compiling for Android?")
set(CROSS OFF CACHE BOOL "Cross-compiling?")
set(CROSS_TARGET OFF CACHE STRING "Cross-compilation target")

# Allow including our modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Mimic system db targets
add_library(tor INTERFACE)

# Set up fake targets
add_library(tor_lib STATIC IMPORTED GLOBAL)

target_link_libraries(tor INTERFACE tor_lib)

# Set up prefix
set(TOR_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "TOR_PREFIX: ${TOR_PREFIX}")

set(TOR_PATH ${TOR_PREFIX}/usr/local/lib/libtor.a)
message(STATUS "TOR_PATH ${TOR_PATH}")

set(TOR_INCLUDE_PATH ${TOR_PREFIX}/usr/local/include PARENT_SCOPE)

# Set up Tor target
include(BuildTor)

# Set import locations
set_target_properties(tor_lib PROPERTIES IMPORTED_LOCATION ${TOR_PATH})

# Set include locations
target_include_directories(tor BEFORE INTERFACE ${TOR_PREFIX}/usr/local/Tor.${TOR_BUILD_VERSION_SHORT}/include)

# Add fake targets to common target
add_dependencies(tor_lib tor)
