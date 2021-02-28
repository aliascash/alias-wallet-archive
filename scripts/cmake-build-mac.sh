#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2019-10-10 HLXEasy
#
# This script can be used to build Alias on and for Mac using CMake
#
# ===========================================================================

# ===========================================================================
# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ownLocation}" || die 1 "Unable to cd into own location ${ownLocation}"
. ./include/helpers_console.sh
_init
. ./include/handle_buildconfig.sh

##### ### # Global definitions # ### ########################################
##### ### # Mac Qt # ### ####################################################
if [[ -z "${MAC_QT_DIR}" ]]; then
    MAC_QT_DIR=${QT_INSTALLATION_PATH}/${QT_VERSION_MAC}/clang_64
fi

MAC_QT_LIBRARYDIR=${MAC_QT_DIR}/lib

##### ### # Boost # ### #####################################################
# Trying to find required Homebrew Boost libs
if [[ -z "${BOOST_VERSION_MAC}" ]]; then
    BOOST_VERSION_MAC=1.73.0
fi
BOOST_INCLUDEDIR=/usr/local/Cellar/boost/${BOOST_VERSION_MAC}/include
BOOST_LIBRARYDIR=/usr/local/Cellar/boost/${BOOST_VERSION_MAC}/lib
BOOST_REQUIRED_LIBS='chrono filesystem iostreams program_options system thread regex date_time atomic'
# regex date_time atomic

##### ### # Qt # ### ########################################################
# Location of Qt will be resolved by trying to find required Qt libs
QT_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Qt
QT_REQUIRED_LIBS='Core Widgets WebView WebChannel WebSockets QuickWidgets Quick Gui Qml Network'

##### ### # BerkeleyDB # ### ################################################
# Location of archive will be resolved like this:
# ${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz
BERKELEYDB_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/BerkeleyDB

##### ### # OpenSSL # ### ###################################################
# Location of archive will be resolved like this:
# ${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
#OPENSSL_ARCHIVE_LOCATION=https://mirror.viaduck.org/openssl
OPENSSL_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/OpenSSL

##### ### # EventLib # ### ##################################################
# Location of archive will be resolved like this:
# ${LIBEVENT_ARCHIVE_LOCATION}/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz
LIBEVENT_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/EventLib

##### ### # ZLib # ### ######################################################
# Location of archive will be resolved like this:
# ${LIBZ_ARCHIVE_LOCATION}/v${LIBZ_BUILD_VERSION}.tar.gz
LIBZ_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/ZLib

##### ### # XZLib # ### #####################################################
# Location of archive will be resolved like this:
# ${LIBXZ_ARCHIVE_LOCATION}/xz-${LIBXZ_BUILD_VERSION}.tar.gz
LIBXZ_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/XZLib

##### ### # Tor # ### #######################################################
# Location of archive will be resolved like this:
# ${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION}.tar.gz
TOR_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Tor
TOR_RESOURCE_ARCHIVE=Tor.libraries.MacOS.zip

BUILD_DIR=cmake-build-cmdline-mac

helpMe() {
    echo "

    Helper script to build Alias wallet and daemon using CMake.
    Required library archives will be downloaded once and will be used
    on subsequent builds.

    Default download location is ~/Archives. You can change this by
    modifying '${ownLocation}/scripts/.buildconfig'.

    Usage:
    ${0} [options]

    Optional parameters:
    -c <cores-to-use>
        The amount of cores to use for build. If not using this option
        the script determines the available cores on this machine.
        Not used for build steps of external libraries like OpenSSL or
        LevelDB.
    -d  Do _not_ build Alias but only the dependencies. Used to prepare
        build slaves a/o builder docker images.
    -f  Perform fullbuild by cleanup all generated data from previous
        build runs.
    -g  Build GUI (Qt) components
    -o  Perfom only Alias fullbuild. Only the alias buildfolder
        will be wiped out before. All other folders stay in place.
    -p <path-to-build-and-install-dependencies-directory>
        Build/install the required dependencies onto the given directory.
        With this option the required dependencies could be located outside
        the Git clone. This is useful
        a) to have them only once at the local machine, even if working
           with multiple Git clones and
        b) to have them separated from the project itself, which is a must
           if using Qt Creator. Otherwise Qt Creator always scans and finds
           all the other content again and again.
        Given value must be an absolute path or relative to the root of the
        Git clone.
    -t  Build with included Tor
    -h  Show this help

    "
}

# ===== Start of homebrew functions ==========================================
checkHomebrew() {
    info ""
    info "Homebrew:"
    if homebrewVersion=$(brew --version 2>/dev/null) ; then
        # Show only the first line of the version output
        info " -> Found ${homebrewVersion/$'\n'*/}"
    else
        error " -> Homebrew not found!"
        error "    You need to install homebrew and after that BerkeleyDB v4 and Boost:"
        error "    brew install berkeley-db@4 boost"
        error ""
        die 40 "Stopping build because of missing Homebrew"
    fi
}
# ===== End of homebrew functions ============================================

# ============================================================================

# ===== Start of openssl functions ===========================================
checkOpenSSLArchive() {
    if [[ -e "${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz" ]]; then
        info " -> Using OpenSSL archive ${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz"
    else
        OPENSSL_ARCHIVE_URL=https://mirror.viaduck.org/openssl/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
        info " -> Downloading OpenSSL archive ${OPENSSL_ARCHIVE_URL}"
        if [[ ! -e ${OPENSSL_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${OPENSSL_ARCHIVE_LOCATION}
        fi
        cd ${OPENSSL_ARCHIVE_LOCATION}
        wget ${OPENSSL_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

# For OpenSSL we're using a fork of https://github.com/viaduck/openssl-cmake
# with some slight modifications for Alias
checkOpenSSLClone() {
    local currentDir=$(pwd)
    cd ${ownLocation}/../external
    if [[ -d openssl-cmake ]]; then
        info " -> Updating openssl-cmake clone"
        cd openssl-cmake
        git pull --prune
    else
        info " -> Cloning openssl-cmake"
        git clone --branch alias https://github.com/aliascash/openssl-cmake.git openssl-cmake
    fi
    cd "${currentDir}"
}

checkOpenSSLBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/openssl
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/openssl || die 1 "Unable to cd into ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/openssl"

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DBUILD_OPENSSL=ON \
    -DOPENSSL_ARCHIVE_LOCATION=${OPENSSL_ARCHIVE_LOCATION} \
    -DOPENSSL_BUILD_VERSION=${OPENSSL_BUILD_VERSION} \
    -DOPENSSL_API_COMPAT=0x00908000L \
    -DOPENSSL_ARCHIVE_HASH=${OPENSSL_ARCHIVE_HASH} \
    ${ownLocation}/../external/openssl-cmake
EOM

    echo "=============================================================================="
    echo "Executing the following CMake cmd:"
    echo "${cmd}"
    echo "=============================================================================="
    #    read a
    ${cmd}
    #    read a

    info ""
    info " -> Building with ${CORES_TO_USE} cores:"
    CORES_TO_USE=${CORES_TO_USE} cmake \
        --build . \
        -- \
        -j "${CORES_TO_USE}"

    rtc=$?
    info ""
    if [[ ${rtc} = 0 ]]; then
        info " -> Finished openssl build and install"
    else
        die ${rtc} " => OpenSSL build failed with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkOpenSSL() {
    info ""
    info "OpenSSL:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libssl.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libssl.a, skip build"
    else
        checkOpenSSLArchive
        checkOpenSSLClone
        checkOpenSSLBuild
    fi
}
# ===== End of openssl functions =============================================

# ============================================================================

# ===== Start of berkeleydb functions ========================================
checkBerkeleyDB() {
    info ""
    info "BerkeleyDB:"
    info " -> Searching required Homebrew BerkeleyDB package"
    berkeleydbVersion=$(brew ls --versions berkeley-db@4)
    if [ $? -eq 0 ] ; then
        info " -> Found ${berkeleydbVersion}"
    else
        error " -> Required BerkeleyDB dependency not found!"
        error "    You need to install homebrew and install BerkeleyDB v4:"
        error "    brew install berkeley-db@4"
        error ""
        die 41 "Stopping build because of missing Boost"
    fi
}
# ===== End of berkeleydb functions ==========================================

# ============================================================================

# ===== Start of boost functions =============================================
checkBoost() {
    info ""
    info "Boost:"
    info " -> Searching required Homebrew Boost package"
    boostVersion=$(brew ls --versions boost)
    if [ $? -eq 0 ] ; then
        info " -> Found ${boostVersion}"
    else
        error " -> Required Boost dependencies not found!"
        error "    You need to install homebrew and install Boost:"
        error "    brew install boost"
        error ""
        die 42 "Stopping build because of missing Boost"
    fi
}
# ===== End of boost functions ===============================================

# ============================================================================

# ===== Start of Qt functions ================================================
checkQt() {
    info ""
    info "Qt:"
    info " -> Searching required Qt libs"
    qtComponentMissing=false
    if [[ -d ${MAC_QT_LIBRARYDIR} ]]; then
        # libQt5Quick.so
        #        for currentQtDependency in ${QT_REQUIRED_LIBS} ; do
        #            if [[ -n $(find ${MAC_QT_LIBRARYDIR}/ -name "libQt5${currentQtDependency}*") ]] ; then
        #                info " -> ${currentQtDependency}: OK"
        #            else
        #                warning " -> ${currentQtDependency}: Not found!"
        #                qtComponentMissing=true
        #            fi
        #        done
        info " -> Found Qt library directory ${MAC_QT_LIBRARYDIR}"
        info "    Detailed check for required libs needs to be implemented"
    else
        info " -> Qt library directory ${MAC_QT_LIBRARYDIR} not found"
        qtComponentMissing=true
    fi
    if ${qtComponentMissing}; then
        error " -> Qt ${QT_VERSION_MAC} not found!"
        error "    You need to install Qt ${QT_VERSION_MAC}"
        error ""
        die 43 "Stopping build because of missing Qt component(s)"
    fi
}
# ===== End of Qt functions ==================================================

# ============================================================================

# ===== Start of libevent functions ==========================================
checkEventLibArchive() {
    if [[ -e "${LIBEVENT_ARCHIVE_LOCATION}/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz" ]]; then
        info " -> Using EventLib archive ${LIBEVENT_ARCHIVE_LOCATION}/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz"
    else
        LIBEVENT_ARCHIVE_URL=https://github.com/libevent/libevent/releases/download/release-${LIBEVENT_BUILD_VERSION}-stable/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz
        info " -> Downloading EventLib archive ${LIBEVENT_ARCHIVE_URL}"
        if [[ ! -e ${LIBEVENT_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${LIBEVENT_ARCHIVE_LOCATION}
        fi
        cd ${LIBEVENT_ARCHIVE_LOCATION}
        wget ${LIBEVENT_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkEventLibClone() {
    local currentDir=$(pwd)
    cd ${ownLocation}/../external
    if [[ -d libevent ]]; then
        info " -> Updating libevent clone"
        cd libevent
        git pull --prune
    else
        info " -> Cloning libevent"
        git clone https://github.com/libevent/libevent.git libevent
    fi
    cd "${currentDir}"
}

checkEventLibBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libevent
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libevent

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DOPENSSL_ROOT_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib;${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/include \
    -DZLIB_INCLUDE_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/include \
    -DZLIB_LIBRARY_RELEASE=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib \
    -DEVENT__DISABLE_TESTS=ON \
    -DEVENT__DISABLE_MBEDTLS=ON \
    -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local \
    ${ownLocation}/../external/libevent
EOM

    echo "=============================================================================="
    echo "Executing the following CMake cmd:"
    echo "${cmd}"
    echo "=============================================================================="
    #    read a
    ${cmd}
    #    read a

    info ""
    info " -> Building with ${CORES_TO_USE} cores:"
    CORES_TO_USE=${CORES_TO_USE} cmake \
        --build . \
        -- \
        -j "${CORES_TO_USE}"

    rtc=$?
    info ""
    if [[ ${rtc} = 0 ]]; then
        info " -> Finished libevent build, installing..."
        make install || die $? " => Error during installation of libevent"
    else
        die ${rtc} " => libevent build failed with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkEventLib() {
    info ""
    info "EventLib:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libevent.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libevent.a, skip build"
    else
        checkEventLibClone
        checkEventLibBuild
    fi
}
# ===== End of libevent functions ============================================

# ============================================================================

# ===== Start of leveldb functions ===========================================
checkLevelDBClone() {
    local currentDir=$(pwd)
    cd ${ownLocation}/../external
    if [[ -d leveldb ]]; then
        info " -> Updating LevelDB clone"
        cd leveldb
        git pull --prune
    else
        info " -> Cloning LevelDB"
        git clone https://github.com/google/leveldb.git leveldb
        cd leveldb
    fi
    info " -> Checkout release ${LEVELDB_VERSION}"
    git checkout ${LEVELDB_VERSION_TAG}
    cd "${currentDir}"
}

checkLevelDBBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libleveldb
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libleveldb

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/local \
    ${ownLocation}/../external/leveldb
EOM

    echo "=============================================================================="
    echo "Executing the following CMake cmd:"
    echo "${cmd}"
    echo "=============================================================================="
    #    read a
    ${cmd}
    #    read a

    info ""
    info " -> Building with ${CORES_TO_USE} cores:"
    CORES_TO_USE=${CORES_TO_USE} cmake \
        --build . \
        -- \
        -j "${CORES_TO_USE}"

    rtc=$?
    info ""
    if [[ ${rtc} = 0 ]]; then
        info " -> Finished libevent build, installing..."
        make install || die $? "Error during installation of libleveldb"
    else
        die ${rtc} " => libleveldb build failed with return code ${rtc}"
    fi
    #    read a
    cd - >/dev/null
}

checkLevelDB() {
    info ""
    info "LevelDB:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/local/lib/libleveldb.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/local/lib/libleveldb.a, skip build"
    else
        checkLevelDBClone
        checkLevelDBBuild
    fi
}
# ===== End of leveldb functions =============================================

# ============================================================================

# ===== Start of libzstd functions ===========================================
checkZStdLibArchive() {
    if [[ -e "${LIBZ_ARCHIVE_LOCATION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz" ]]; then
        info " -> Using ZLib archive ${LIBZ_ARCHIVE_LOCATION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz"
    else
        LIBZ_ARCHIVE_URL=https://github.com/facebook/zstd/releases/download/v${LIBZ_BUILD_VERSION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz
        info " -> Downloading ZLib archive ${LIBZ_ARCHIVE_URL}"
        if [[ ! -e ${LIBZ_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${LIBZ_ARCHIVE_LOCATION}
        fi
        cd ${LIBZ_ARCHIVE_LOCATION}
        wget ${LIBZ_ARCHIVE_URL}
        cd - >/dev/null
    fi
    cd ${ownLocation}/../external
    if [[ -d libzstd ]]; then
        info " -> Directory external/libzstd already existing. Remove it to extract it again"
    else
        info " -> Extracting zstd-${LIBZ_BUILD_VERSION}.tar.gz..."
        tar xzf ${LIBZ_ARCHIVE_LOCATION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz
        mv zstd-${LIBZ_BUILD_VERSION} libzstd
    fi
    cd - >/dev/null
}

checkZStdLibBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libzstd
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libzstd

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DLIBZ_ARCHIVE_LOCATION=${LIBZ_ARCHIVE_LOCATION} \
    -DLIBZ_BUILD_VERSION=${LIBZ_BUILD_VERSION} \
    -DLIBZ_BUILD_VERSION_SHORT=${LIBZ_BUILD_VERSION%.*} \
    -DLIBZ_ARCHIVE_HASH=${LIBZ_ARCHIVE_HASH} \
    -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local \
    ${ownLocation}/../external/libzstd/build/cmake
EOM

    echo "=============================================================================="
    echo "Executing the following CMake cmd:"
    echo "${cmd}"
    echo "=============================================================================="
    #    read a
    ${cmd}
    #    read a

    info ""
    info " -> Building with ${CORES_TO_USE} cores:"
    CORES_TO_USE=${CORES_TO_USE} cmake \
        --build . \
        -- \
        -j "${CORES_TO_USE}"

    rtc=$?
    info ""
    if [[ ${rtc} = 0 ]]; then
        info " -> Finished libzstd build, installing..."
        make install || die $? "Error during installation of libzstd"
    else
        die ${rtc} " => libzstd build failed with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkZStdLib() {
    info ""
    info "ZStdLib:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libzstd.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libzstd.a, skip build"
    else
        checkZStdLibArchive
        checkZStdLibBuild
    fi
}
# ===== End of libzstd functions =============================================

# ============================================================================

# ===== Start of libxz functions =============================================
checkXZLibArchive() {
    if [[ -e "${LIBXZ_ARCHIVE_LOCATION}/xz-${LIBXZ_BUILD_VERSION}.tar.gz" ]]; then
        info " -> Using XZLib archive ${LIBXZ_ARCHIVE_LOCATION}/xz-${LIBXZ_BUILD_VERSION}.tar.gz"
    else
        LIBXZ_ARCHIVE_URL=https://tukaani.org/xz/xz-${LIBXZ_BUILD_VERSION}.tar.gz
        info " -> Downloading XZLib archive ${LIBZ_ARCHIVE_URL}"
        if [[ ! -e ${LIBXZ_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${LIBXZ_ARCHIVE_LOCATION}
        fi
        cd ${LIBXZ_ARCHIVE_LOCATION}
        wget ${LIBXZ_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkXZLibBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libxz
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libxz

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DLIBXZ_ARCHIVE_LOCATION=${LIBXZ_ARCHIVE_LOCATION} \
    -DLIBXZ_BUILD_VERSION=${LIBXZ_BUILD_VERSION} \
    -DLIBXZ_BUILD_VERSION_SHORT=${LIBXZ_BUILD_VERSION%.*} \
    -DLIBXZ_ARCHIVE_HASH=${LIBXZ_ARCHIVE_HASH} \
    ${ownLocation}/../external/libxz-cmake
EOM

    echo "=============================================================================="
    echo "Executing the following CMake cmd:"
    echo "${cmd}"
    echo "=============================================================================="
    #    read a
    ${cmd}
    #    read a

    info ""
    info " -> Building with ${CORES_TO_USE} cores:"
    CORES_TO_USE=${CORES_TO_USE} cmake \
        --build . \
        -- \
        -j "${CORES_TO_USE}"

    rtc=$?
    info ""
    if [[ ${rtc} = 0 ]]; then
        info " -> Finished libxz build and install"
    else
        die ${rtc} " => libxz build failed with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkXZLib() {
    info ""
    info "XZLib:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/liblzma.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/liblzma.a, skip build"
    else
        checkXZLibArchive
        checkXZLibBuild
    fi
}
# ===== End of libxz functions ===============================================

# ============================================================================

# ===== Start of tor functions ===============================================
checkTorArchive() {
    if [[ -e "${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION}.tar.gz" ]]; then
        info " -> Using Tor archive ${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION}.tar.gz"
    else
        TOR_ARCHIVE_URL=https://github.com/torproject/tor/archive/tor-${TOR_BUILD_VERSION}.tar.gz
        info " -> Downloading Tor archive ${TOR_ARCHIVE_URL}"
        if [[ ! -e ${TOR_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${TOR_ARCHIVE_LOCATION}
        fi
        cd ${TOR_ARCHIVE_LOCATION}
        wget ${TOR_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkTorBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/tor
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/tor

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DTOR_ARCHIVE_LOCATION=${TOR_ARCHIVE_LOCATION} \
    -DTOR_BUILD_VERSION=${TOR_BUILD_VERSION} \
    -DTOR_BUILD_VERSION_SHORT=${TOR_BUILD_VERSION%.*} \
    -DTOR_ARCHIVE_HASH=${TOR_ARCHIVE_HASH} \
    ${ownLocation}/../external/tor-cmake
EOM

    echo "=============================================================================="
    echo "Executing the following CMake cmd:"
    echo "${cmd}"
    echo "=============================================================================="
    #    read a
    ${cmd}
    #    read a

    info ""
    info " -> Building with ${CORES_TO_USE} cores:"
    CORES_TO_USE=${CORES_TO_USE} cmake \
        --build . \
        -- \
        -j "${CORES_TO_USE}"

    rtc=$?
    info ""
    if [[ ${rtc} = 0 ]]; then
        info " -> Finished tor build and install"
    else
        die ${rtc} " => Tor build failed with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkTor() {
    info ""
    info "Tor:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/bin/tor ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/bin/tor, skip build"
    else
        checkTorArchive
        checkTorBuild
    fi
}

checkTorMacArchive() {
    info ""
    info "Tor:"
    if [[ -e "${TOR_ARCHIVE_LOCATION}/${TOR_RESOURCE_ARCHIVE}" ]]; then
        info " -> Using Tor archive ${TOR_ARCHIVE_LOCATION}/${TOR_RESOURCE_ARCHIVE}"
    else
        TOR_ARCHIVE_URL=https://github.com/aliascash/resources/raw/master/resources/${TOR_RESOURCE_ARCHIVE}
        info " -> Downloading Tor archive ${TOR_RESOURCE_ARCHIVE}"
        if [[ ! -e ${TOR_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${TOR_ARCHIVE_LOCATION}
        fi
        cd ${TOR_ARCHIVE_LOCATION}
        wget ${TOR_ARCHIVE_URL}
        cd - >/dev/null
    fi
}
# ===== End of tor functions =================================================

# ============================================================================

# Determine system
# Determine amount of cores:
if [[ "$OSTYPE" == "linux-gnu" ]]; then
    CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    CORES_TO_USE=$(system_profiler SPHardwareDataType | grep "Total Number of Cores" | tr -s " " | cut -d " " -f 6)
#elif [[ "$OSTYPE" == "cygwin" ]]; then
#    # POSIX compatibility layer and Linux environment emulation for Windows
#elif [[ "$OSTYPE" == "msys" ]]; then
#    # Lightweight shell and GNU utilities compiled for Windows (part of MinGW)
#elif [[ "$OSTYPE" == "win32" ]]; then
#    # I'm not sure this can happen.
#elif [[ "$OSTYPE" == "freebsd"* ]]; then
#    CORES_TO_USE=1
else
    CORES_TO_USE=1
fi

FULLBUILD=false
ENABLE_GUI=false
ENABLE_GUI_PARAMETERS='OFF'
BUILD_ONLY_ALIAS=false
BUILD_ONLY_DEPENDENCIES=false
WITH_TOR=false
GIVEN_DEPENDENCIES_BUILD_DIR=''

while getopts c:dfgop:th? option; do
    case ${option} in
    c) CORES_TO_USE="${OPTARG}" ;;
    d) BUILD_ONLY_DEPENDENCIES=true ;;
    f) FULLBUILD=true ;;
    g)
        ENABLE_GUI=true
        ENABLE_GUI_PARAMETERS="ON -DQT_CMAKE_MODULE_PATH=${MAC_QT_LIBRARYDIR}/cmake"
        ;;
    o) BUILD_ONLY_ALIAS=true ;;
    p) GIVEN_DEPENDENCIES_BUILD_DIR="${OPTARG}" ;;
    t) WITH_TOR=true ;;
    h | ?) helpMe && exit 0 ;;
    *) die 90 "invalid option \"${OPTARG}\"" ;;
    esac
done

# Go to alias-wallet repository root directory
cd ..

# ============================================================================
# Handle given path to dependency location
if [[ -n "${GIVEN_DEPENDENCIES_BUILD_DIR}" ]] ; then
    # ${GIVEN_DEPENDENCIES_BUILD_DIR} is set,
    # so store given path on build configuration
    if [[ "${GIVEN_DEPENDENCIES_BUILD_DIR}" = /* ]]; then
        # Absolute path given
        DEPENDENCIES_BUILD_DIR=${GIVEN_DEPENDENCIES_BUILD_DIR}
    else
        # Relative path given
        DEPENDENCIES_BUILD_DIR=${ownLocation}/../${GIVEN_DEPENDENCIES_BUILD_DIR}
    fi
    storeDependenciesBuildDir "${DEPENDENCIES_BUILD_DIR}"
fi

# ============================================================================
# If ${DEPENDENCIES_BUILD_DIR} is empty, no path to the dependencies is given
# or stored on script/.buildproperties. In this case use default location
# inside of Git clone
if [[ -z "${DEPENDENCIES_BUILD_DIR}" ]] ; then
    DEPENDENCIES_BUILD_DIR=${ownLocation}/..
fi

info ""
info "Building/using dependencies on/from directory:"
info " -> ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}"

if [[ ! -d ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR} ]]; then
    info ""
    info "Creating dependency build directory ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}"
    mkdir -p "${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}"
    info " -> Done"
fi

cd "${DEPENDENCIES_BUILD_DIR}" || die 1 "Unable to cd into ${DEPENDENCIES_BUILD_DIR}"
DEPENDENCIES_BUILD_DIR=$(pwd)

# ============================================================================
# Handle which parts should be build
cd "${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}" || die 1 "Unable to cd into ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}"
if ${FULLBUILD}; then
    info ""
    info "Cleanup leftovers from previous build run"
    rm -rf ./*
    info " -> Done"
fi

# ============================================================================
# Check a/o build requirements/dependencies
checkHomebrew
checkBoost
checkBerkeleyDB
checkLevelDB
checkOpenSSL
if ${WITH_TOR}; then
    checkXZLib
    checkZStdLib
    checkEventLib
    checkTor
else
    checkTorMacArchive
fi
if ${ENABLE_GUI}; then
    checkQt
fi

# ============================================================================
# Only dependencies should be build, so exit here
if ${BUILD_ONLY_DEPENDENCIES}; then
    info ""
    info "Checked a/o built all required dependencies."
    exit
fi

# ============================================================================
# Dependencies are ready. Go ahead with the main project
ALIAS_BUILD_DIR=${ownLocation}/../${BUILD_DIR}/aliaswallet
if [[ ! -d ${ALIAS_BUILD_DIR} ]]; then
    info ""
    info "Creating Alias build directory ${ALIAS_BUILD_DIR}"
    mkdir -p "${ALIAS_BUILD_DIR}"
    info " -> Done"
fi
cd "${ALIAS_BUILD_DIR}" || die 1 "Unable to cd into Alias build directory '${ALIAS_BUILD_DIR}'"

# Update $ALIAS_BUILD_DIR with full path
ALIAS_BUILD_DIR=$(pwd)

# If requested, cleanup leftovers from previous build
if [[ ${FULLBUILD} = true ]] || [[ ${BUILD_ONLY_ALIAS} = true ]]; then
    info ""
    info "Cleanup leftovers from previous build run"
    rm -rf ./*
    info " -> Done"
fi

info ""
info "Generating Alias build configuration"
read -r -d '' cmd <<EOM
cmake \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    \
    -DENABLE_GUI=${ENABLE_GUI_PARAMETERS} \
    \
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR} \
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR} \
    \
    -DBerkeleyDB_ROOT_DIR=/usr/local/opt/berkeley-db@4 \
    -DBERKELEYDB_INCLUDE_DIR=/usr/local/opt/berkeley-db@4/include \
    \
    -Dleveldb_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/local/lib/cmake/leveldb \
    -DLEVELDB_INCLUDE_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/local/include \
    \
    -DOPENSSL_ROOT_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib;${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/include \
    \
    -DTOR_ARCHIVE=${TOR_ARCHIVE_LOCATION}/${TOR_RESOURCE_ARCHIVE}
EOM

# Insert additional parameters
# Not used for now
#if ${WITH_TOR} ; then
#    read -r -d '' cmd << EOM
#${cmd} \
#    -DWITH_TOR=ON
#EOM
#fi

# Finalize build cmd
read -r -d '' cmd <<EOM
${cmd} \
    ${ALIAS_BUILD_DIR}/../..
EOM

echo "=============================================================================="
echo "Executing the following CMake cmd:"
echo "${cmd}"
echo "=============================================================================="
#read a
${cmd}
#read a

info ""
info "Building with ${CORES_TO_USE} cores:"
CORES_TO_USE=${CORES_TO_USE} cmake \
    --build . \
    -- \
    -j "${CORES_TO_USE}"

rtc=$?
info ""
if [[ ${rtc} = 0 ]]; then
    info " -> Finished"
else
    die 50 " => Failed with return code ${rtc}"
fi

# Debug build:
#DEPLOY_QT_BINARY_TYPE_OPTION="-use-debug-libs"
DEPLOY_QT_BINARY_TYPE_OPTION=''

if [[ -e ${MAC_QT_DIR}/bin/macdeployqt ]] ; then
    info ""
    info "Creating dmg:"
    cd "${ALIAS_BUILD_DIR}" || die 1 "Unable to cd into ${ALIAS_BUILD_DIR}"
    ${MAC_QT_DIR}/bin/macdeployqt \
        Alias.app \
        -qmldir="${ownLocation}"/../src/qt/res \
        -always-overwrite \
        -verbose=1 \
        "${DEPLOY_QT_BINARY_TYPE_OPTION}"
    ${MAC_QT_DIR}/bin/macdeployqt \
        Alias.app \
        -dmg \
        -always-overwrite \
        -verbose=1
    info " -> Alias.dmg created"
else
    die 23 "${MAC_QT_DIR}/bin/macdeployqt not found, unable to create dmg!"
fi

info ""
info "Performing post build steps:"
info "============================"

cd "${ALIAS_BUILD_DIR}" || die 1 "Unable to cd into Alias build directory '${ALIAS_BUILD_DIR}'"
cp Alias.dmg Alias.dmg.bak

info "Change permision of .dmg file"
hdiutil convert "Alias.dmg" -format UDRW -o "Alias_Rw.dmg"
info " -> Done"

info "Mount it and save the device"
PATH_AT_VOLUME=/Volumes/Alias
DEVICE=$(hdiutil attach -readwrite -noverify "Alias_Rw.dmg" | grep ${PATH_AT_VOLUME} | awk '{print $1}')
info " -> Done (${DEVICE})"

sleep 2

info "Create symbolic link to application folder"
pushd "$PATH_AT_VOLUME"
ln -s /Applications
popd
info " -> Done"

info "Copy background image in to package"
mkdir "$PATH_AT_VOLUME"/.background
cp "${ownLocation}"/../src/osx/app-slide-arrow.png "$PATH_AT_VOLUME"/.background/
info " -> Done"

info "Resize window, set background, change icon size, place icons in the right position, etc."
echo '
    tell application "Finder"
    tell disk "Alias"   ## check Path inside cd /Volume/
        open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set the bounds of container window to {400, 100, 1200, 520}
            set viewOptions to the icon view options of container window
            set arrangement of viewOptions to not arranged
            set icon size of viewOptions to 200
            set text size of viewOptions to 16
            set background picture of viewOptions to file ".background:app-slide-arrow.png"
            set position of item "Alias.app" of container window to {180, 200}
            set position of item "Applications" of container window to {620, 200}
        close
        open
        update without registering applications
        delay 2
    end tell
    end tell
' | osascript
info " -> Done"

sync

info "Unmount"
hdiutil detach "${DEVICE}"
info " -> Done"

info "Cleanup and convert"
rm -f "Alias.dmg"
hdiutil convert "Alias_Rw.dmg" -format UDZO -o "Alias.dmg"
rm -f "Alias_Rw.dmg"
info " -> Done"

info " -> Finished: ${ALIAS_BUILD_DIR}/Alias.dmg"

cd "${callDir}" || die 1 "Unable to cd back to where we came from (${callDir})"
