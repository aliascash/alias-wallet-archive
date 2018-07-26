cmake_minimum_required (VERSION 2.6)
project (Spectre)

# The version number.
set (CLIENT_VERSION_MAJOR      2)
set (CLIENT_VERSION_MINOR      0)
set (CLIENT_VERSION_REVISION   3)
set (CLIENT_VERSION_BUILD      0)
set (CLIENT_VERSION_GIT_SHA1   "---")

# Compiler flags
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++0x")

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
include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/leveldb/helpers
        ${CMAKE_CURRENT_LIST_DIR}/obj
        ${CMAKE_CURRENT_LIST_DIR}/test
        ${CMAKE_CURRENT_LIST_DIR}/test/other
        ${CMAKE_CURRENT_LIST_DIR}/test/data
        ${CMAKE_CURRENT_LIST_DIR}/wordlists
        ${CMAKE_CURRENT_LIST_DIR}/qt
        ${CMAKE_CURRENT_LIST_DIR}/qt/res
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/css\n/qt/res/css/fonts
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/images
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/images/avatars
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/icons
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/css
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/md5
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/identicon
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/boostrapv3
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/boostrapv3/css
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/boostrapv3/js
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/boostrapv3/fonts
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/framework
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/markdown
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/shajs
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/pnglib
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/iscroll
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/jquery
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/classie
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/pace
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/contextMenu
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/jquery-scrollbar
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/jdenticon
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/qrcode
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/emojione
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/emojione/assets
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/emojione/assets/svg
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/emojione/assets/css
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/jquery-transit
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/footable
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/jquery-ui
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/plugins/jquery-ui/images
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/js
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/js/pages
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/img
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/img/progress
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/img/avatars
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/icons
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/fonts
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/fonts/Framework-icon
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/fonts/FontAwesome
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/fonts/Montserrat
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/assets/fonts/Footable
        ${CMAKE_CURRENT_LIST_DIR}/qt/res/src
        ${CMAKE_CURRENT_LIST_DIR}/qt/locale
        ${CMAKE_CURRENT_LIST_DIR}/qt/forms
        ${CMAKE_CURRENT_LIST_DIR}/qt/test
        ${CMAKE_CURRENT_LIST_DIR}/lz4
        ${CMAKE_CURRENT_LIST_DIR}/json
        ${CMAKE_CURRENT_LIST_DIR}/xxhash
        ${CMAKE_CURRENT_LIST_DIR}/obj-test
)
