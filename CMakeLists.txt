cmake_minimum_required(VERSION 3.4)
add_executable(Spectrecoin src)

project(Spectrecoin
        VERSION 2.2.0.8
        LANGUAGES
        CXX
        )

# The version number.
set(CLIENT_VERSION_MAJOR 2)
set(CLIENT_VERSION_MINOR 2)
set(CLIENT_VERSION_REVISION 0)
set(CLIENT_VERSION_BUILD 1)
set(CLIENT_VERSION_GIT_SHA1 "---")

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

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

# This is needed if your Boost version is newer than your CMake version
# or if you have an old version of CMake (<3.5)
if (NOT TARGET Boost::filesystem)
    add_library(Boost::filesystem IMPORTED INTERFACE)
    set_property(TARGET Boost::filesystem PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
    set_property(TARGET Boost::filesystem PROPERTY
            INTERFACE_LINK_LIBRARIES ${Boost_LIBRARIES})
endif ()

target_link_libraries(Spectrecoin PUBLIC Boost::filesystem)


# Include Boost as an imported target
#find_package(Boost REQUIRED
#        COMPONENTS
#        chrono
#        filesystem
#        iostreams
#        program-options
#        system
#        thread
#        )
#add_library(boost INTERFACE IMPORTED)
#set_property(TARGET boost PROPERTY
#        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

# Add the binary tree to the search path for include files
# so that we will find SpectreConfig.h
include_directories("${PROJECT_BINARY_DIR}" "leveldb/include/" "db4.8/dbinc/")

#target_link_libraries(Spectrecoin
#        Boost::chrono
#        Boost::filesystem
#        Boost::iostreams
#        Boost::program-options
#        Boost::system
#        Boost::thread
#        )

add_subdirectory(src)
