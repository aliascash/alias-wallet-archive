cmake_minimum_required (VERSION 2.6)
project (Spectre)

# The version number.
set (Spectre_VERSION_MAJOR 2)
set (Spectre_VERSION_MINOR 0)
set (Spectre_VERSION_BUGFIX 0)
set (Spectre_GIT_SHA1 "---")

# Add the binary tree to the search path for include files
# so that we will find SpectreConfig.h
include_directories("${PROJECT_BINARY_DIR}")

# Add the executable
add_executable(Spectre "")

include(src/CMakeLists.txt)
