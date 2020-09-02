# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.14)
project(Alias
        VERSION 4.0
        LANGUAGES
        CXX
        C
        )

# The version number.
set(CLIENT_VERSION_MAJOR 4)
set(CLIENT_VERSION_MINOR 3)
set(CLIENT_VERSION_REVISION 0)
set(CLIENT_VERSION_BUILD 0)
set(CLIENT_VERSION_GIT_SHA1 "---")
set(VERSION_STRING "${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_REVISION}.${CLIENT_VERSION_BUILD}")
#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# https://www.jetbrains.com/help/clion/cmake-profile.html
# adding two custom build types to the cached value
# variable CMAKE_CONFIGURATION_TYPES should be defined before other definitions:
set(CMAKE_CONFIGURATION_TYPES "DebugAndroid;ReleaseAndroid" CACHE STRING "" FORCE)

#set(ANDROID 1)
set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/android")

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

    # For compile issues with vcpkg
    add_compile_definitions(NOMINMAX)
endif ()

if (ANDROID)
    if(DEFINED ENV{BOOST_ROOT})
        message(STATUS "Getting settings from environment")
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
        set(BOOST_ROOT "$ENV{BOOST_ROOT}")
        set(BOOST_INCLUDEDIR "$ENV{BOOST_INCLUDEDIR}")
        set(Boost_INCLUDE_DIR "$ENV{BOOST_INCLUDEDIR}")
        set(BOOST_LIBRARYDIR "$ENV{BOOST_LIBRARYDIR}")
        set(Boost_LIBRARY_DIR "$ENV{BOOST_LIBRARYDIR}")
        set(BerkeleyDB_ROOT_DIR "$ENV{BerkeleyDB_ROOT_DIR}")
        set(BERKELEYDB_INCLUDE_DIR "$ENV{BERKELEYDB_INCLUDE_DIR}")
        set(leveldb_DIR "$ENV{leveldb_DIR}")
        set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_ROOT_DIR}")
        set(ENABLE_GUI ON)
    endif()
    message(STATUS "Configuring for Android")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-deprecated-declarations -Wno-dangling-else")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")

    # https://github.com/arsenm/sanitizers-cmake/issues/13#issuecomment-315532253
    # Use "-fPIC" / "-fPIE" for all targets by default, including static libs
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    # CMake doesn't add "-pie" by default for executables (CMake issue #14983)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring on/for Linux")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring on/for macOS")

    # Handle build type
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(DEPLOY_QT_BINARY_TYPE_OPTION "-use-debug-libs")
    else()
        set(DEPLOY_QT_BINARY_TYPE_OPTION "")
    endif()

    # Handle direct access linker warnings with Boost
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fvisibility-inlines-hidden -fvisibility=hidden")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Configuring on/for Windows")
    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc-")
    add_definitions(-DBOOST_ALL_DYN_LINK)
    add_definitions("/wd4244")
    add_definitions("/wd4267")
    add_definitions(-DWIN32_LEAN_AND_MEAN)

    # Handle build type
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(DEPLOY_QT_BINARY_TYPE_OPTION "--debug")
    else()
        set(DEPLOY_QT_BINARY_TYPE_OPTION "--release")
    endif()
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

message(STATUS "Building in '${CMAKE_BUILD_TYPE}' mode")

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
message(STATUS "=== Qt =====================================================================")
if (ENABLE_GUI)
    set(Qt5Core_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Core")
    set(Qt5Widgets_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Widgets")
    set(Qt5WebView_DIR "${QT_CMAKE_MODULE_PATH}/Qt5WebView")
    set(Qt5WebChannel_DIR "${QT_CMAKE_MODULE_PATH}/Qt5WebChannel")
    set(Qt5WebSockets_DIR "${QT_CMAKE_MODULE_PATH}/Qt5WebSockets")
    set(Qt5QuickWidgets_DIR "${QT_CMAKE_MODULE_PATH}/Qt5QuickWidgets")
    set(Qt5Quick_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Quick")
    set(Qt5Svg_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Svg")

    # Additional entries:
    set(Qt5_DIR "${QT_CMAKE_MODULE_PATH}/Qt5")
    set(Qt5Gui_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Gui")
    set(Qt5Qml_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Qml")
    set(Qt5Network_DIR "${QT_CMAKE_MODULE_PATH}/Qt5Network")
    set(Qt5QmlModels_DIR "${QT_CMAKE_MODULE_PATH}/Qt5QmlModels")

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

    find_package(Qt5Gui)
    message(STATUS "Qt5Gui version: ${Qt5Gui_VERSION}")
    get_target_property(QtGui_location Qt5::Gui LOCATION)

    find_package(Qt5Widgets)
    message(STATUS "Qt5Widgets version: ${Qt5Widgets_VERSION}")
    get_target_property(QtWidgets_location Qt5::Widgets LOCATION)

    find_package(Qt5WebView)
    message(STATUS "Qt5WebView version: ${Qt5WebView_VERSION}")
    get_target_property(QtWebView_location Qt5::WebView LOCATION)

    find_package(Qt5WebChannel)
    message(STATUS "Qt5WebChannel version: ${Qt5WebChannel_VERSION}")
    get_target_property(Qt5WebChannel_location Qt5::WebChannel LOCATION)

    find_package(Qt5WebSockets)
    message(STATUS "Qt5WebSockets version: ${Qt5WebSockets_VERSION}")
    get_target_property(Qt5WebSockets_location Qt5::WebSockets LOCATION)

    find_package(Qt5QuickWidgets)
    message(STATUS "Qt5QuickWidgets version: ${Qt5QuickWidgets_VERSION}")
    get_target_property(QtQuickWidgets_location Qt5::QuickWidgets LOCATION)

    find_package(Qt5Quick)
    message(STATUS "Qt5Quick version: ${Qt5Quick_VERSION}")
    get_target_property(QtQuick_location Qt5::Quick LOCATION)

    find_package(Qt5Svg)
    message(STATUS "Qt5Svg version: ${Qt5Svg_VERSION}")
    get_target_property(QtSvg_location Qt5::Svg LOCATION)

    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # Retrieve the absolute path to qmake and then use that path
        # to find the macdeployqt binary
        get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

        # Put delivery result into dedicated directory
        set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

        if (NOT TOR_ARCHIVE)
            include(DownloadFile)
            set(TOR_ARCHIVE "Tor.libraries.MacOS.zip")
            download_file(https://github.com/spectrecoin/resources/raw/master/resources/Tor.libraries.MacOS.zip ${TOR_ARCHIVE} SHA1 0be391e0cad9e93d33c226d6bff719f568b56093)
        endif ()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Retrieve the absolute path to qmake and then use that path
        # to find the windeployqt binary
        get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

        # Put delivery result into dedicated directory
        set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/delivery)

        if (NOT TOR_ARCHIVE)
            include(DownloadFile)
            set(TOR_ARCHIVE "${CMAKE_BINARY_DIR}/Tor.libraries.Win64.zip")
            download_file(https://github.com/spectrecoin/resources/raw/master/resources/Tor.libraries.Win64.zip ${TOR_ARCHIVE} SHA1 ac40fae3eee1040d7ffab64b9b7b7356ca75b37c)
        endif()
endif()

else()
    message(STATUS "Building without UI, skipping Qt detection")
endif ()


# ============================================================================================
# Requires lib and include dir at once, separated by ";":
#     -DOPENSSL_ROOT_DIR=${BUILD_DIR}/usr/local/lib;${BUILD_DIR}/usr/local/include
message(STATUS "=== OpenSSL ================================================================")
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL)

message(STATUS "OPENSSL_FOUND: ${OPENSSL_FOUND}")
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OPENSSL_SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
message(STATUS "OPENSSL_VERSION: ${OPENSSL_VERSION}")

# ============================================================================================
# Requires:
#     -DBerkeleyDB_ROOT_DIR=${BUILD_DIR}/libdb/libdb-install
#     -DBERKELEYDB_INCLUDE_DIR=${BUILD_DIR}/libdb/libdb-install/include
message(STATUS "=== Berkeley DB ============================================================")
find_package(BerkeleyDB)

# ============================================================================================
# Requires:
#     -Dleveldb_DIR=${BUILD_DIR}/usr/local/lib/cmake/leveldb
message(STATUS "=== LevelDB ================================================================")
find_package(leveldb)

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
