cmake_minimum_required(VERSION 3.9)
project(Spectrecoin
        VERSION 2.2.2
        LANGUAGES
        CXX
        C
        )

# The version number.
set(CLIENT_VERSION_MAJOR 2)
set(CLIENT_VERSION_MINOR 2)
set(CLIENT_VERSION_REVISION 2)
set(CLIENT_VERSION_BUILD 0)
set(CLIENT_VERSION_GIT_SHA1 "---")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring on/for Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring on/for macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Configuring on/for Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "AIX")
    message(STATUS "Configuring on/for IBM AIX")
else()
    message(STATUS "Configuring on/for ${CMAKE_SYSTEM_NAME}")
endif()

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif ()

# Include own cmake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif ()

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# ============================================================================================
message(STATUS "=== Boost ==================================================================")
# Env var BOOST_ROOT should point to local Boost installation
#
#set(BOOST_ROOT C:/boost_1_69_0/)
#set(Boost_DEBUG on)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.65.1 REQUIRED COMPONENTS
        chrono
        filesystem
        iostreams
        program_options
        system
        thread
        )
message(STATUS "Boost version: ${Boost_VERSION}")

# ============================================================================================
message(STATUS "=== Berkeley DB ============================================================")
#
# Use BDB local module
#FIND_PACKAGE(BerkeleyDB)
#IF(DB_INCLUDE_DIR)
#    INCLUDE_DIRECTORIES(${DB_INCLUDE_DIR})
#ELSE(DB_INCLUDE_DIR)
#    MESSAGE(FATAL_ERROR "Could not locate BerkeleyDB")
#ENDIF(DB_INCLUDE_DIR)

# ============================================================================================
message(STATUS "=== Qt =====================================================================")
# Env vars Qt5Core_DIR, Qt5Widgets_DIR and Qt5WebEngine_DIR should point to
# directory <qt-dir>/msvc2017_64
#
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
message(STATUS "Qt5Core version: ${Qt5Core_VERSION}")
get_target_property(QtCore_location Qt5::Core LOCATION)

find_package(Qt5Widgets)
message(STATUS "Qt5Widgets version: ${Qt5Widgets_VERSION}")
get_target_property(QtWidgets_location Qt5::Widgets LOCATION)

find_package(Qt5WebEngine)
message(STATUS "Qt5WebEngine version: ${Qt5WebEngine_VERSION}")
get_target_property(QtWebEngine_location Qt5::WebEngine LOCATION)

find_package(Qt5WebEngineWidgets)
message(STATUS "Qt5WebEngineWidgets version: ${Qt5WebEngineWidgets_VERSION}")
get_target_property(QtWebEngineWidgets_location Qt5::WebEngineWidgets LOCATION)

# ============================================================================================
message(STATUS "=== OpenSSL ================================================================")
# Env var OPENSSL_ROOT_DIR should point to local OpenSSL installation from
# https://slproweb.com/products/Win32OpenSSL.html
find_package(OpenSSL)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
get_target_property(OpenSSL_location OpenSSL::SSL LOCATION)
get_target_property(OpenSSL_location OpenSSL::Crypto LOCATION)

message(STATUS "OPENSSL_FOUND: ${OPENSSL_FOUND}")
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OPENSSL_SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

# ============================================================================================
message(STATUS "=== LevelDB ================================================================")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/leveldb_upstream/)

# ============================================================================================
#message(STATUS "=== List of all variables ==================================================")
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach ()

# ============================================================================================
message(STATUS "=== Git helper =============================================================")
# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

# ============================================================================================
message(STATUS "=== Dive into subdirs... ===================================================")
add_subdirectory(src)
