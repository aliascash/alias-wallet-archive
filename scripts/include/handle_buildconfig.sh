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
BOOST_HASH=9a2c2819310839ea373f42d69e733c339b4e9a19deab6bfec448281554aa4dbb

##### ### # BerkeleyDB # ### ################################################
BERKELEYDB_BUILD_VERSION=4.8.30
BERKELEYDB_HASH=e0491a07cdb21fb9aa82773bbbedaeb7639cbd0e7f96147ab46141e0045db72a
#BERKELEYDB_BUILD_VERSION=5.0.32
#BERKELEYDB_HASH=...
#BERKELEYDB_BUILD_VERSION=6.2.38
#BERKELEYDB_HASH=...

##### ### # OpenSSL # ### ###################################################
OPENSSL_BUILD_VERSION=1.1.0l
OPENSSL_HASH=74a2f756c64fd7386a29184dc0344f4831192d61dc2481a93a4c5dd727f41148
#OPENSSL_BUILD_VERSION=1.1.1d
#OPENSSL_HASH=...

##### ### # EventLib # ### ##################################################
LIBEVENT_BUILD_VERSION=2.1.11
LIBEVENT_HASH=a65bac6202ea8c5609fd5c7e480e6d25de467ea1917c08290c521752f147283d

##### ### # ZLib # ### ######################################################
LIBZ_BUILD_VERSION=1.2.11
LIBZ_HASH=629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff

##### ### # XZLib # ### #####################################################
LIBXZ_BUILD_VERSION=5.2.4
LIBXZ_HASH=b512f3b726d3b37b6dc4c8570e137b9311e7552e8ccbab4d39d47ce5f4177145

##### ### # Tor # ### #######################################################
TOR_BUILD_VERSION=0.4.1.6
TOR_HASH=ee7adbbc5e30898bc35d9658bbf6a67e4242977175f7bad11c5f1ee0c1010d43

##### ### # Android # ### ###################################################
ANDROID_NDK_VERSION=r20

EOF
fi
. .buildconfig
