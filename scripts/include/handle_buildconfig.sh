#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2019-11-30 HLXEasy
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

##### ### # Android # ### ###################################################
ANDROID_NDK_VERSION=r21d
ANDROID_SDK_VERSION=29
ANDROID_SDK_ROOT=${HOME}/Archives/Android/Sdk

##### ### # Boost # ### #####################################################
BOOST_VERSION=1.73.0
BOOST_ARCHIVE_HASH=9a2c2819310839ea373f42d69e733c339b4e9a19deab6bfec448281554aa4dbb

##### ### # Qt # ### ########################################################
# Path to the folder which contains the Qt installation
# aka version folder to use
QT_INSTALLATION_PATH=${HOME}/Qt

QT_REQUIRED_LIBS='Core Widgets WebView WebChannel WebSockets QuickWidgets Quick Gui Qml Network'

# Qt version to use. In fact the folder right below ${QT_INSTALLATION_PATH}
#QT_VERSION=5.11.3
#QT_ARCHIVE_HASH=unknown
#QT_VERSION=5.12.7
#QT_ARCHIVE_HASH=ce2c5661c028b9de6183245982d7c120
#QT_VERSION=5.12.8
#QT_ARCHIVE_HASH=8ec2a0458f3b8e9c995b03df05e006e4
#QT_VERSION=5.14.2
#QT_ARCHIVE_HASH=b3d2b6d00e6ca8a8ede6d1c9bdc74daf
QT_VERSION=5.15.0
QT_ARCHIVE_HASH=610a228dba6ef469d14d145b71ab3b88

# These are the default Qt versions on the corresponding distributions
QT_VERSION_ANDROID=5.15.2
QT_VERSION_CENTOS_8=5.12.8
QT_VERSION_DEBIAN_BUSTER=5.11.3
QT_VERSION_UBUNTU_1804=5.9.5
QT_VERSION_UBUNTU_1904=5.12.4
QT_VERSION_UBUNTU_1910=5.12.4
QT_VERSION_UBUNTU_2004=5.12.8
QT_VERSION_FEDORA=5.14.2
QT_VERSION_OPENSUSE_TUMBLEWEED=5.15.1

##### ### # Qt (Mac) # ### ##################################################
# Installed Qt version. In fact the folder below /Applications/Qt/
QT_VERSION_MAC=5.12.10

##### ### # BerkeleyDB # ### ################################################
BERKELEYDB_BUILD_VERSION=4.8.30
BERKELEYDB_ARCHIVE_HASH=e0491a07cdb21fb9aa82773bbbedaeb7639cbd0e7f96147ab46141e0045db72a
#BERKELEYDB_BUILD_VERSION=5.0.32
#BERKELEYDB_ARCHIVE_HASH=...
#BERKELEYDB_BUILD_VERSION=6.2.38
#BERKELEYDB_ARCHIVE_HASH=...

##### ### # LevelDB # ### ###################################################
LEVELDB_VERSION=1.22
LEVELDB_VERSION_TAG=78b39d68

##### ### # OpenSSL # ### ###################################################
OPENSSL_BUILD_VERSION=1.1.0l
OPENSSL_ARCHIVE_HASH=74a2f756c64fd7386a29184dc0344f4831192d61dc2481a93a4c5dd727f41148
#OPENSSL_ARCHIVE_HASH=...

##### ### # EventLib # ### ##################################################
LIBEVENT_BUILD_VERSION=2.1.11
LIBEVENT_ARCHIVE_HASH=a65bac6202ea8c5609fd5c7e480e6d25de467ea1917c08290c521752f147283d

##### ### # ZLib # ### ######################################################
#LIBZ_BUILD_VERSION=1.2.11
#LIBZ_ARCHIVE_HASH=629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff
LIBZ_BUILD_VERSION=1.4.4
LIBZ_ARCHIVE_HASH=59ef70ebb757ffe74a7b3fe9c305e2ba3350021a918d168a046c6300aeea9315

##### ### # XZLib # ### #####################################################
LIBXZ_BUILD_VERSION=5.2.4
LIBXZ_ARCHIVE_HASH=b512f3b726d3b37b6dc4c8570e137b9311e7552e8ccbab4d39d47ce5f4177145

##### ### # Tor # ### #######################################################
TOR_BUILD_VERSION=0.4.1.6
TOR_ARCHIVE_HASH=ee7adbbc5e30898bc35d9658bbf6a67e4242977175f7bad11c5f1ee0c1010d43
#TOR_BUILD_VERSION=0.4.1.7
#TOR_ARCHIVE_HASH=f769c8052f0c0f74b9b7bcaff6255f656e313da232bfa8f89d2a165df3868850
#TOR_BUILD_VERSION=0.4.2.5
#TOR_ARCHIVE_HASH=94ad248f4d852a8f38bd8902a12b9f41897c76e389fcd5b8a7d272aa265fd6c9

TOR_BUILD_VERSION_ANDROID=0.4.1.6
TOR_ARCHIVE_HASH_ANDROID=ee7adbbc5e30898bc35d9658bbf6a67e4242977175f7bad11c5f1ee0c1010d43
#TOR_BUILD_VERSION_ANDROID=0.4.2.3-alpha
#TOR_ARCHIVE_HASH_ANDROID=be22b9326093dd6b012377d9e3c456028cd2104e5a454a7773aebf75d44c1ccf
#TOR_BUILD_VERSION_ANDROID=0.4.2.5
#TOR_ARCHIVE_HASH_ANDROID=94ad248f4d852a8f38bd8902a12b9f41897c76e389fcd5b8a7d272aa265fd6c9

EOF
fi
info ""
info "Build configuration:"
info " -> Loading general build configuration"
. .buildconfig

if [ -e "${HOME}/.alias_buildconfig" ] ; then
    info " -> Loading personal build configuration"
    # Personal build config found, so load it
    . ${HOME}/.alias_buildconfig
else
    # Personal build config not existing, create it with all entries commented
    info ""
    warning " -> Personal build configuration not found, creating it now!"
    sed "s/^\([a-zA-Z]\)/#\1/g" .buildconfig > ${HOME}/.alias_buildconfig
    info ""
    info "If you like to modify '${HOME}/.alias_buildconfig',"
    info "you should break script execution now (Ctrl-C)"
    for i in $(seq 10 -1 0) ; do
        info "${i}"
        sleep 1
    done
fi
