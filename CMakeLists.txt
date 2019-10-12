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
set(CLIENT_VERSION_BUILD 3)
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
#    set(ANDROID_ARM_MODE arm)
##    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -fexceptions -g")
##    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fexceptions -g")
#    set(CMAKE_CXX_STANDARD_REQUIRED ON)
#    set(CMAKE_CXX_EXTENSIONS OFF)
#    set(C_STANDARD 11)
#    set(CMAKE_C_STANDARD 11)
#    set(CMAKE_C_STANDARD_REQUIRED ON)
#    set(CMAKE_C_EXTENSIONS OFF)
#    add_compile_definitions(__clang__)
#    add_compile_definitions(__ANDROID__)
##    add_compile_definitions(_POSIX_C_SOURCE)
##    add_compile_definitions(_XOPEN_SOURCE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring on/for Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall")
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
if (ANDROID)
    message(STATUS "=== Boost (Android) ========================================================")
    add_compile_definitions(BOOST_ASIO_ENABLE_OLD_SERVICES)
    add_compile_definitions(BOOST_SPIRIT_THREADSAFE)
    add_compile_definitions(BOOST_THREAD_USE_LIB)
#    add_compile_definitions(BOOST_THREAD_POSIX)
#    add_compile_definitions(BOOST_HAS_THREADS)
    message(STATUS "= boost_chrono")
    add_library(boost_chrono SHARED IMPORTED)
    set_target_properties(boost_chrono PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_chrono.so
            )

    message(STATUS "= boost_filesystem")
    add_library(boost_filesystem SHARED IMPORTED)
    set_target_properties(boost_filesystem PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_filesystem.so
            )

    message(STATUS "= boost_iostreams")
    add_library(boost_iostreams SHARED IMPORTED)
    set_target_properties(boost_iostreams PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_iostreams.so
            )

    message(STATUS "= boost_program_options")
    add_library(boost_program_options SHARED IMPORTED)
    set_target_properties(boost_program_options PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_program_options.so
            )

    message(STATUS "= boost_system")
    add_library(boost_system SHARED IMPORTED)
    set_target_properties(boost_system PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_system.so
            )

    message(STATUS "= boost_thread")
    add_library(boost_thread SHARED IMPORTED)
    set_target_properties(boost_thread PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_thread.so
            )

    message(STATUS "= boost_timer")
    add_library(boost_timer SHARED IMPORTED)
    set_target_properties(boost_timer PROPERTIES IMPORTED_LOCATION
            ${MY_BOOST_LIBS_DIR}/${ANDROID_ABI}/libboost_timer.so
            )

    message(STATUS "= Boost include dir")
    include_directories(${MY_BOOST_INC_DIR}
            )
else ()
    message(STATUS "=== Boost ==================================================================")
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME ON)
    find_package(Boost 1.65.1 REQUIRED COMPONENTS
            chrono
            filesystem
            iostreams
            program_options
            system
            thread
            )
    message(STATUS "Boost_VERSION:      ${Boost_VERSION}")
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
    message(STATUS "Boost_LIBRARIES:    ${Boost_LIBRARIES}")
endif ()


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
set(OPENSSL_ENABLE_TESTS ON)
set(OPENSSL_BUILD_VERSION OpenSSL_1_1_0g)
add_subdirectory(external/openssl-cmake)


# ============================================================================================
message(STATUS "=== Berkeley DB ============================================================")
add_subdirectory(external/berkeleydb-cmake)


# ============================================================================================
message(STATUS "=== LevelDB ================================================================")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/leveldb/)


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
