cmake_minimum_required (VERSION 2.6)
project (Spectre)

# The version number.
set (CLIENT_VERSION_MAJOR      2)
set (CLIENT_VERSION_MINOR      0)
set (CLIENT_VERSION_REVISION   3)
set (CLIENT_VERSION_GIT_SHA1   "---")

# Determine current Git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC CLIENT_VERSION_GIT_SHA1)

# Add the binary tree to the search path for include files
# so that we will find SpectreConfig.h
include_directories("${PROJECT_BINARY_DIR}" "leveldb/include/" "db4.8/dbinc/" )

# Add the executable
add_executable(Spectre "")

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
