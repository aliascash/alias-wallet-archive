cmake_minimum_required(VERSION 3.9)
project(Spectrecoin
        VERSION 2.2.0.8
        LANGUAGES
        CXX
        C
        )

# The version number.
set(CLIENT_VERSION_MAJOR 2)
set(CLIENT_VERSION_MINOR 2)
set(CLIENT_VERSION_REVISION 0)
set(CLIENT_VERSION_BUILD 8)
set(CLIENT_VERSION_GIT_SHA1 "---")

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#set(cxx_have_stdheaders "#define HAVE_CXX_STDHEADERS")

# Include own cmake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif ()

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# === Boost ==================================================================
#set(BOOST_ROOT C:/boost_1_69_0/)
#set(Boost_DEBUG on)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.67 REQUIRED COMPONENTS
        chrono
        filesystem
        iostreams
        program_options
        system
        thread
        )
message(STATUS "Boost version: ${Boost_VERSION}")

# === Berkeley DB ============================================================
# Use BDB local module
FIND_PACKAGE(BerkeleyDB)
#IF(DB_INCLUDE_DIR)
#    INCLUDE_DIRECTORIES(${DB_INCLUDE_DIR})
#ELSE(DB_INCLUDE_DIR)
#    MESSAGE(FATAL_ERROR "Could not locate BerkeleyDB")
#ENDIF(DB_INCLUDE_DIR)

# === Qt =====================================================================
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS qt/forms)
# Handle .qrc files added as target sources at build time and invoke rcc accordingly
set(CMAKE_AUTORCC ON)

find_package(Qt5Core)
get_target_property(QtCore_location Qt5::Core LOCATION)

find_package(Qt5Widgets)
get_target_property(QtWidgets_location Qt5::Widgets LOCATION)

find_package(Qt5WebEngine)
get_target_property(QtWebEngine_location Qt5::WebEngine LOCATION)

# === Include Git helper =====================================================
# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

add_subdirectory(src)
add_subdirectory(leveldb)
