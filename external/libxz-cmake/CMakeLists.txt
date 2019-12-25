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
project(libxz-cmake)

set(LIBXZ_ARCHIVE_LOCATION OFF CACHE STRING "Location of XZLib archive")
set(LIBXZ_BUILD_VERSION OFF CACHE STRING "XZLib version to build")
set(CROSS_ANDROID OFF CACHE BOOL "Cross-compiling for Android?")
set(CROSS OFF CACHE BOOL "Cross-compiling?")
set(CROSS_TARGET OFF CACHE STRING "Cross-compilation target")

# Allow including our modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Mimic system db targets
add_library(libxz INTERFACE)

# Set up fake targets
add_library(lib_xz STATIC IMPORTED GLOBAL)

target_link_libraries(libxz INTERFACE lib_xz)

# Set up prefix
set(LIBXZ_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "LIBXZ_PREFIX: ${LIBXZ_PREFIX}")

set(LIBXZ_PATH ${LIBXZ_PREFIX}/usr/local/lib/liblzma.a)
message(STATUS "LIBXZ_PATH ${LIBXZ_PATH}")

set(LIBXZ_INCLUDE_PATH ${LIBXZ_PREFIX}/usr/local/include PARENT_SCOPE)

# Set up XZLib target
include(BuildXZLib)

# Set import locations
set_target_properties(lib_xz PROPERTIES IMPORTED_LOCATION ${LIBXZ_PATH})

# Set include locations
target_include_directories(libxz BEFORE INTERFACE ${LIBXZ_PREFIX}/usr/local/include)

# Add fake targets to common target
add_dependencies(lib_xz libxzExternal)