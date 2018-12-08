cmake_minimum_required(VERSION 3.4)

project(Spectrecoin
        VERSION 2.0.8
        LANGUAGES
        CXX
        )

# The version number.
set (CLIENT_VERSION_MAJOR      2)
set (CLIENT_VERSION_MINOR      2)
set (CLIENT_VERSION_REVISION   0)
set (CLIENT_VERSION_BUILD      1)
set (CLIENT_VERSION_GIT_SHA1   "---")

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17")

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Include Boost as an imported target
find_package(Boost REQUIRED
        COMPONENTS
        chrono
        filesystem
        iostreams
        program-options
        system
        thread
        )
add_library(boost INTERFACE IMPORTED)
set_property(TARGET boost PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

# Add the binary tree to the search path for include files
# so that we will find SpectreConfig.h
include_directories("${PROJECT_BINARY_DIR}" "leveldb/include/" "db4.8/dbinc/" )

target_link_libraries(example
        Boost::chrono
        Boost::filesystem
        Boost::iostreams
        Boost::program-options
        Boost::system
        Boost::thread
        )

add_subdirectory(src)
