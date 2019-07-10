cmake_minimum_required(VERSION 3.9)
project(Spectrecoin
        VERSION 3.0
        LANGUAGES
        CXX
        C
        )

# The version number.
set(CLIENT_VERSION_MAJOR 3)
set(CLIENT_VERSION_MINOR 0)
set(CLIENT_VERSION_REVISION 10)
set(CLIENT_VERSION_BUILD 1)
set(CLIENT_VERSION_GIT_SHA1 "---")

set(CMAKE_VERBOSE_MAKEFILE ON)

#set(ANDROID 1)
if(ANDROID)
    message(STATUS "Configuring for Android")
    set(ANDROID_ARM_MODE arm)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring on/for Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring on/for macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Configuring on/for Windows")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc-")
    add_definitions(-DBOOST_ALL_DYN_LINK)
    add_definitions("/wd4244")
    add_definitions("/wd4267")
elseif(CMAKE_SYSTEM_NAME STREQUAL "AIX")
    message(STATUS "Configuring on/for IBM AIX")
else()
    message(STATUS "Configuring on/for ${CMAKE_SYSTEM_NAME}")
endif()

# Compiler flags
set(CMAKE_CXX_STANDARD 17)
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

    # For compile issues with vcpkg
    add_compile_definitions(NOMINMAX)
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
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME ON)
#    set(BOOST_ROOT       /home/starwarsfan/spectre/Boost-for-Android/build/out/armeabi-v7a)
#    set(BOOST_LIBRARYDIR /home/starwarsfan/spectre/Boost-for-Android/build/out/armeabi-v7a/lib)
#    set(BOOST_INCLUDEDIR /home/starwarsfan/spectre/Boost-for-Android/build/out/armeabi-v7a/include/boost-1_69)
#    set(BOOST_INCLUDEDIR /usr/include)
#    set(Boost_NO_SYSTEM_PATHS TRUE)
#    set(Boost_ADDITIONAL_VERSIONS 1_69 1.69)
#    set(Boost_COMPILER -clang)
#    set(Boost_ARCHITECTURE -a32)
#    set(Boost_LIB_VERSION 1_69)
#    set(Boost_LIB_PREFIX lib)
#    set(Boost_USE_DEBUG_LIBS OFF)
#    set(Boost_USE_DEBUG_LIBS OFF)
#    set(Boost_USE_STATIC_LIBS ON)
#    set(Boost_USE_MULTITHREADED ON)
#    set(Boost_INCLUDE_DIRS /usr/include)
    set(Boost_INCLUDE_DIRS ~/Boost/Boost-for-Android/build/out/armeabi-v7a/include/boost-1_69)
    set(Boost_LIBRARIES ~/Boost/Boost-for-Android/build/out/armeabi-v7a/lib/libboost_chrono-clang-mt-a32-1_69.a;
            ~/Boost/Boost-for-Android/build/out/armeabi-v7a/lib/libboost_filesystem-clang-mt-a32-1_69.a;
            ~/Boost/Boost-for-Android/build/out/armeabi-v7a/lib/libboost_iostreams-clang-mt-a32-1_69.a;
            ~/Boost/Boost-for-Android/build/out/armeabi-v7a/lib/libboost_program_options-clang-mt-a32-1_69.a;
            ~/Boost/Boost-for-Android/build/out/armeabi-v7a/lib/libboost_system-clang-mt-a32-1_69.a;
            ~/Boost/Boost-for-Android/build/out/armeabi-v7a/lib/libboost_thread-clang-mt-a32-1_69.a;
            )
#    set(Boost_USE_STATIC_RUNTIME ON)
#    set(Boost_DEBUG on)
    message(STATUS "BOOST_ROOT: ${BOOST_ROOT}")
    message(STATUS "BOOST_LIBRARYDIR: ${BOOST_LIBRARYDIR}")
    message(STATUS "BOOST_INCLUDEDIR: ${BOOST_INCLUDEDIR}")
else()
    message(STATUS "=== Boost ==================================================================")
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
endif()
message(STATUS "Boost_VERSION:      ${Boost_VERSION}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES:    ${Boost_LIBRARIES}")



# ============================================================================================
if (ANDROID)
    message(STATUS "=== Qt disabled for Android build ==========================================")
else()
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
if (ANDROID)
    message(STATUS "=== OpenSSL (Android) ======================================================")
    # Set env var LOCAL_OPENSSL_DIR to the folder where the include dir and
    # libcrypto.a as well as libssl.a are located
    set(OPENSSL_ROOT_DIR ${LOCAL_OPENSSL_DIR})
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(OPENSSL_INCLUDE_DIR ${LOCAL_OPENSSL_DIR}/include)
    set(OPENSSL_CRYPTO_LIBRARY ${LOCAL_OPENSSL_DIR}/libcrypto.a)
    set(OPENSSL_SSL_LIBRARY ${LOCAL_OPENSSL_DIR}/libssl.a)
else()
    message(STATUS "=== OpenSSL ================================================================")
endif()
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
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/leveldb/)

# ============================================================================================
if (ANDROID)
    message(STATUS "=== Berkeley DB (Andoird) ==================================================")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/db5.x)
else()
    message(STATUS "=== Berkeley DB ============================================================")
    #set(BerkeleyDB_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/db4.8)
    find_package(BerkeleyDB REQUIRED)
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
