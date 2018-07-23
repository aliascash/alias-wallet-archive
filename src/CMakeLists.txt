# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC Spectre_GIT_SHA1)

# Configure a header file to pass some of the CMake settings
# to the source code like version and Git hash
configure_file (
        "${CMAKE_CURRENT_LIST_DIR}/spectreconfig.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/spectreconfig.h"
)

target_sources(Spectre
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/spectre.cxx"
)

include(${CMAKE_CURRENT_LIST_DIR}/json/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/lz4/CMakeLists.txt)
#include(${CMAKE_CURRENT_LIST_DIR}/qt/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/win/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/wordlists/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/xxhash/CMakeLists.txt)
