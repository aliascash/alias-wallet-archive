#!/bin/bash
# ===========================================================================
#
# Created: 2019-11-30 HLXEasy
#
# This script can be used to build Spectrecoin using CMake
#
# ===========================================================================

if [[ ! -e .buildconfig ]] ; then
    cat << EOF > .buildconfig
##### ### # Global definitions # ### ########################################
# Windows with msys2
#ARCHIVES_ROOT_DIR=C:/msys64${HOME}/Archives
# Linux
# Do not use '~' as later steps might not be able to expand this!
ARCHIVES_ROOT_DIR=${HOME}/Archives

##### ### # Boost # ### #####################################################
BOOST_VERSION=1.69.0

##### ### # BerkeleyDB # ### ################################################
BERKELEYDB_BUILD_VERSION=4.8.30
#BERKELEYDB_BUILD_VERSION=5.0.32
#BERKELEYDB_BUILD_VERSION=6.2.38

##### ### # OpenSSL # ### ###################################################
OPENSSL_BUILD_VERSION=1.1.0l
#OPENSSL_BUILD_VERSION=1.1.1d

##### ### # EventLib # ### ##################################################
EVENTLIB_BUILD_VERSION=2.1.11

##### ### # ZLib # ### ######################################################
ZLIB_BUILD_VERSION=1.2.11

##### ### # XZLib # ### #####################################################
XZLIB_BUILD_VERSION=5.2.4

##### ### # Tor # ### #######################################################
TOR_BUILD_VERSION=0.4.1.6

##### ### # Android # ### ###################################################
ANDROID_NDK_VERSION=r20

EOF
fi
. .buildconfig
