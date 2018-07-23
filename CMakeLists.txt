cmake_minimum_required (VERSION 2.6)
project (Spectre)

# The version number.
set (Spectre_VERSION_MAJOR 2)
set (Spectre_VERSION_MINOR 0)
set (Spectre_VERSION_BUGFIX 0)

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file (
        "${PROJECT_SOURCE_DIR}/src/spectreconfig.h.in"
        "${PROJECT_BINARY_DIR}/src/spectreconfig.h"
)

# Add the binary tree to the search path for include files
# so that we will find SpectreConfig.h
include_directories("${PROJECT_BINARY_DIR}")

# add the executable
add_executable(Spectre ${PROJECT_SOURCE_DIR}/src/spectre.cxx)
