cmake_minimum_required(VERSION 3.14)
project(Spectrecoin
        VERSION 4.0
        LANGUAGES
        CXX
        C
        )

# The version number.
set(CLIENT_VERSION_MAJOR 4)
set(CLIENT_VERSION_MINOR 1)
set(CLIENT_VERSION_REVISION 0)
set(CLIENT_VERSION_BUILD 5)
set(CLIENT_VERSION_GIT_SHA1 "---")

set(CMAKE_VERBOSE_MAKEFILE ON)

# https://www.jetbrains.com/help/clion/cmake-profile.html
# adding two custom build types to the cached value
# variable CMAKE_CONFIGURATION_TYPES should be defined before other definitions:
set(CMAKE_CONFIGURATION_TYPES "DebugAndroid;ReleaseAndroid" CACHE STRING "" FORCE)

#set(ANDROID 1)

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

    # For compile issues with vcpkg
    add_compile_definitions(NOMINMAX)
endif ()

if (ANDROID)
    message(STATUS "Configuring for Android")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring on/for Linux")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring on/for macOS")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Configuring on/for Windows")
    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc-")
    add_definitions(-DBOOST_ALL_DYN_LINK)
    add_definitions("/wd4244")
    add_definitions("/wd4267")
elseif (CMAKE_SYSTEM_NAME STREQUAL "AIX")
    message(STATUS "Configuring on/for IBM AIX")
else ()
    message(STATUS "Configuring on/for ${CMAKE_SYSTEM_NAME}")
endif ()

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif ()

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# ============================================================================================
message(STATUS "=== Helper functions =======================================================")
# Include own cmake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Determine current Git hash
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)


# ============================================================================================
# Env var BOOST_ROOT should point to local Boost installation
message(STATUS "=== Boost ==================================================================")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_DEBUG_LIBS OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS ON)  # only find release libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost 1.65.1 REQUIRED COMPONENTS
        atomic
        chrono
        date_time
        filesystem
        iostreams
        program_options
        regex
        system
        thread
        )
message(STATUS "Boost_VERSION:      ${Boost_VERSION}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES:    ${Boost_LIBRARIES}")


# ============================================================================================
if (ANDROID)
    message(STATUS "=== Qt disabled for Android build ==========================================")
else ()
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
endif ()


# ============================================================================================
message(STATUS "=== OpenSSL ================================================================")
set(BUILD_OPENSSL ON)
set(OPENSSL_ENABLE_TESTS OFF)
add_subdirectory(external/openssl-cmake)


# ============================================================================================
message(STATUS "=== Berkeley DB ============================================================")
add_subdirectory(external/berkeleydb-cmake)

# ============================================================================================
message(STATUS "=== LevelDB ================================================================")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/leveldb/)

# ============================================================================================
if (WITH_TOR)
    add_compile_definitions(WITH_TOR)

    message(STATUS "============================================================================")
    message(STATUS "=== Build with Tor =========================================================")
    message(STATUS "============================================================================")
    # ============================================================================================
    message(STATUS "=== EventLib ===============================================================")
    add_subdirectory(external/libevent-cmake)

    # ============================================================================================
    message(STATUS "=== ZLib ===================================================================")
    add_subdirectory(external/libz-cmake)

    # ============================================================================================
    message(STATUS "=== XZLib ==================================================================")
    add_subdirectory(external/libxz-cmake)

    # ============================================================================================
    message(STATUS "=== Tor ====================================================================")
    add_subdirectory(external/tor-cmake)
endif()

# ============================================================================================
message(STATUS "=== Dive into subdirs... ===================================================")
add_subdirectory(src)

# ============================================================================================
#message(STATUS "=== List of all variables ==================================================")
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach ()
