cmake_minimum_required(VERSION 3.4)
project(Spectrecoin
        VERSION 2.2.0.8
        LANGUAGES
        CXX
        )

# The version number.
set(CLIENT_VERSION_MAJOR 2)
set(CLIENT_VERSION_MINOR 2)
set(CLIENT_VERSION_REVISION 0)
set(CLIENT_VERSION_BUILD 8)
set(CLIENT_VERSION_GIT_SHA1 "---")

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Include own cmake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif ()

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
#set(BOOST_ROOT C:/boost_1_68_0/)
#set(BOOST_LIBRARYDIR C:/boost_1_68_0/)
find_package(Boost 1.67 REQUIRED COMPONENTS
        chrono
        filesystem
        iostreams
        program_options
        system
        thread
        )
message(STATUS "Boost version: ${Boost_VERSION}")

# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

include_directories("${PROJECT_BINARY_DIR}" "leveldb/include/" "db4.8/dbinc/")

add_subdirectory(src)
