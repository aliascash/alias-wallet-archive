#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2019-10-22 HLXEasy
#
# This script can be used to build Alias for Android using CMake
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
##### ### # Android # ### ###################################################
ANDROID_NDK_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Android
ANDROID_NDK_ROOT=${ANDROID_NDK_ARCHIVE_LOCATION}/android-ndk-${ANDROID_NDK_VERSION}
ANDROID_TOOLCHAIN_CMAKE=${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake
ANDROID_ARCH=x86_64
ANDROID_ABI=x86_64
ANDROID_API=${ANDROID_API_X86_64}

##### ### # Android Qt # ### ################################################
ANDROID_QT_DIR=${QT_INSTALLATION_PATH}/${QT_VERSION_ANDROID}/android
ANDROID_QT_LIBRARYDIR=${ANDROID_QT_DIR}/lib

##### ### # Boost # ### #####################################################
# Location of Boost will be resolved by trying to find required Boost libs
BOOST_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Boost
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

BUILD_DIR=cmake-build-cmdline-android${ANDROID_API}_${ANDROID_ARCH}

helpMe() {
    echo "

    Helper script to build Alias wallet and daemon using CMake.
    Required library archives will be downloaded once and will be used
    on subsequent builds. This includes also Android NDK.

    Default download location is ~/Archives. You can change this by
    modifying '${ownLocation}/scripts/.buildconfig'.

    Usage:
    ${0} [options]

    Optional parameters:
    -c <cores-to-use>
        The amount of cores to use for build. If not using this option
        the script determines the available cores on this machine.
        Not used for build steps of external libraries like OpenSSL or
        BerkeleyDB.
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
    -s  Use Qt from system
    -t  Build with included Tor
    -h  Show this help

    "
}

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
    -DANDROID=1 \
    -DCROSS_ANDROID=ON \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
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
checkBerkeleyDBArchive() {
    if [[ -e "${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz" ]]; then
        info " -> Using BerkeleyDB archive ${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz"
    else
        BERKELEYDB_ARCHIVE_URL=https://download.oracle.com/berkeley-db/db-${BERKELEYDB_BUILD_VERSION}.tar.gz
        info " -> Downloading BerkeleyDB archive ${BERKELEYDB_ARCHIVE_URL}"
        if [[ ! -e ${BERKELEYDB_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${BERKELEYDB_ARCHIVE_LOCATION}
        fi
        cd ${BERKELEYDB_ARCHIVE_LOCATION} || die 1 "Unable to cd into ${BERKELEYDB_ARCHIVE_LOCATION}"
        wget ${BERKELEYDB_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkBerkeleyDBBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb || die 1 "Unable to cd into ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb"

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DANDROID=1 \
    -DCROSS_ANDROID=ON \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
    -DBERKELEYDB_ARCHIVE_LOCATION=${BERKELEYDB_ARCHIVE_LOCATION} \
    -DBERKELEYDB_BUILD_VERSION=${BERKELEYDB_BUILD_VERSION} \
    -DBERKELEYDB_BUILD_VERSION_SHORT=${BERKELEYDB_BUILD_VERSION%.*} \
    -DBERKELEYDB_ARCHIVE_HASH=${BERKELEYDB_ARCHIVE_HASH} \
    -DCMAKE_INSTALL_PREFIX=/libdb-install \
    ${ownLocation}/../external/berkeleydb-cmake
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
        info " -> Finished BerkeleyDB (libdb) build and install"
    else
        die ${rtc} " => BerkeleyDB (libdb) build failed with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkBerkeleyDB() {
    info ""
    info "BerkeleyDB:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install/lib/libdb.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install/lib/libdb.a, skip build"
    else
        checkBerkeleyDBArchive
        checkBerkeleyDBBuild
    fi
}
# ===== End of berkeleydb functions ==========================================

# ============================================================================

# ===== Start of boost functions =============================================
checkBoostArchive() {
    local currentDir=$(pwd)
    if [[ ! -e ${BOOST_ARCHIVE_LOCATION} ]]; then
        mkdir -p ${BOOST_ARCHIVE_LOCATION}
    fi
    cd ${BOOST_ARCHIVE_LOCATION} || die 1 "Unable to cd into ${BOOST_ARCHIVE_LOCATION}"
    if [[ ! -e "boost_${BOOST_VERSION//./_}.tar.gz" ]]; then
        info " -> Downloading Boost archive"
        wget https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
    else
        info " -> Using existing Boost archive"
    fi
}

buildBoost() {
    info " -> Building Boost on ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}"
    cd "${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}" || die 1 "Unable to cd into ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}"
    info " -> Cleanup before extraction"
    rm -rf boost_${BOOST_VERSION//./_}
    info " -> Extracting Boost archive"
    tar xzf ${BOOST_ARCHIVE_LOCATION}/boost_${BOOST_VERSION//./_}.tar.gz
    info " -> Building Boost"
    cd boost_${BOOST_VERSION//./_} || die 1 "Unable to cd into boost_${BOOST_VERSION//./_}"
    "${ownLocation}"/build-boost-for-android.sh -v ${BOOST_VERSION} -a ${ANDROID_ARCH} -p ${ANDROID_API} -n ${ANDROID_NDK_ROOT} -l "${BOOST_REQUIRED_LIBS// /,}"
    cd "${currentDir}" || die 1 "Unable to cd into ${currentDir}"
}

checkBoost() {
    info ""
    info "Boost:"
    info " -> Searching required static Boost libs"
    BOOST_ROOT=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/boost_${BOOST_VERSION//./_}
    BOOST_INCLUDEDIR=${BOOST_ROOT}/include
    BOOST_LIBRARYDIR=${BOOST_ROOT}/lib
    boostBuildRequired=false
    if [[ -d ${BOOST_LIBRARYDIR} ]]; then
        for currentBoostDependency in ${BOOST_REQUIRED_LIBS}; do
            if [[ -e ${BOOST_LIBRARYDIR}/libboost_${currentBoostDependency}.a ]]; then
                info " -> ${currentBoostDependency}: OK"
            else
                warning " => ${currentBoostDependency}: Not found!"
                boostBuildRequired=true
            fi
        done
    else
        warning " => Boost library directory ${BOOST_LIBRARYDIR} not found!"
        boostBuildRequired=true
    fi
    if ${boostBuildRequired} ; then
        checkBoostArchive
        buildBoost
    else
        info " => All Boost requirements found"
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
    if [[ -d ${ANDROID_QT_LIBRARYDIR} ]]; then
        # libQt5Quick.so
        for currentQtDependency in ${QT_REQUIRED_LIBS}; do
            if [[ -n $(find ${ANDROID_QT_LIBRARYDIR}/ -name "libQt5${currentQtDependency}_${ANDROID_ABI}.so") ]]; then
                info " -> ${currentQtDependency}: OK"
            else
                warning " -> ${currentQtDependency}: Not found!"
                qtComponentMissing=true
            fi
        done
    else
        info " -> Qt library directory ${ANDROID_QT_LIBRARYDIR} not found"
        qtComponentMissing=true
    fi
    if ${qtComponentMissing}; then
        error " -> Qt ${QT_VERSION}: Not all required components found!"
        error ""
        die 43 "Stopping build because of missing Qt"

        # Maybe used later: Build Qt ourself
        local currentDir=$(pwd)
        if [[ ! -e ${QT_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${QT_ARCHIVE_LOCATION}
        fi
        cd ${QT_ARCHIVE_LOCATION}
        if [[ ! -e "qt-everywhere-src-${QT_VERSION}.tar.xz" ]]; then
            info " -> Downloading Qt archive"
            wget https://download.qt.io/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/single/qt-everywhere-src-${QT_VERSION}.tar.xz
        else
            info " -> Using existing Qt archive"
        fi
        info " -> Verifying archive checksum"
        determinedMD5Sum=$(md5sum qt-everywhere-src-${QT_VERSION}.tar.xz | cut -d ' ' -f 1)
        if [[ "${determinedMD5Sum}" != "${QT_ARCHIVE_HASH}" ]]; then
            warning " => Checksum of downloaded archive not matching expected value of ${QT_ARCHIVE_HASH}: ${determinedMD5Sum}"
        else
            info " -> Archive checksum ok"
        fi
        info " -> Cleanup before extraction"
        rm -rf qt-everywhere-src-${QT_VERSION}
        info " -> Extracting Qt archive"
        tar xf qt-everywhere-src-${QT_VERSION}.tar.xz
        info " -> Configuring Qt build"
        cd qt-everywhere-src-${QT_VERSION}
        ./configure \
            -xplatform android-clang \
            --disable-rpath \
            -nomake tests \
            -nomake examples \
            -android-ndk ${ANDROID_NDK_ROOT} \
            -android-sdk ${ANDROID_SDK_ROOT} \
            -android-arch ${ANDROID_ABI} \
            -android-ndk-platform android-${ANDROID_API} \
            -no-warnings-are-errors \
            -opensource \
            -confirm-license \
            -silent \
            -prefix ${ANDROID_QT_INSTALLATION_DIR} || die 23 "Error during Qt configure step"
        info " -> Building Qt"
        make -j"${CORES_TO_USE}" || die 24 "Error during Qt build step"
        info " -> Installing Qt"
        make install || die 25 "Error during Qt install step"
        cd "${currentDir}"
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
    -DANDROID=1 \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DANDROID_ABI=${ANDROID_ABI} \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    \
    -DOPENSSL_ROOT_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib;${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/include \
    -DZLIB_INCLUDE_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/include \
    -DZLIB_LIBRARY_RELEASE=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib \
    -DEVENT__DISABLE_TESTS=ON \
    -DEVENT__DISABLE_MBEDTLS=ON
EOM

    # Disable Thread support if Android API is lower than 28
    # See https://github.com/libevent/libevent/issues/949
    if [[ ${ANDROID_API} -lt 28 ]]; then
        read -r -d '' cmd <<EOM
${cmd} \
    -DEVENT__DISABLE_THREAD_SUPPORT=ON
EOM
    fi

    # Finalize build cmd
    read -r -d '' cmd <<EOM
${cmd} \
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
    -DANDROID=1 \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DANDROID_ABI=${ANDROID_ABI} \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    \
    -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local \
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
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libleveldb.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libleveldb.a, skip build"
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
    -DANDROID=1 \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
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
    -DANDROID=1 \
    -DCROSS_ANDROID=ON \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
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
    if [[ -e "${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION_ANDROID}.tar.gz" ]]; then
        info " -> Using Tor archive ${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION_ANDROID}.tar.gz"
    else
        #        TOR_ARCHIVE_URL=https://github.com/torproject/tor/archive/tor-${TOR_BUILD_VERSION}.tar.gz
        TOR_ARCHIVE_URL=https://github.com/guardianproject/tor/archive/tor-${TOR_BUILD_VERSION_ANDROID}.tar.gz
        info " -> Downloading Tor archive ${TOR_ARCHIVE_URL}"
        if [[ ! -e ${TOR_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${TOR_ARCHIVE_LOCATION}
        fi
        cd ${TOR_ARCHIVE_LOCATION}
        wget ${TOR_ARCHIVE_URL} -O tor-${TOR_BUILD_VERSION_ANDROID}.tar.gz
        cd - >/dev/null
    fi
}

checkTorBuild() {
    mkdir -p ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/tor
    cd ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/tor

    info " -> Generating build configuration"
    read -r -d '' cmd <<EOM
cmake \
    -DANDROID=1 \
    -DCROSS_ANDROID=ON \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
    -DTOR_ARCHIVE_LOCATION=${TOR_ARCHIVE_LOCATION} \
    -DTOR_BUILD_VERSION=${TOR_BUILD_VERSION_ANDROID} \
    -DTOR_BUILD_VERSION_SHORT=${TOR_BUILD_VERSION%.*} \
    -DTOR_ARCHIVE_HASH=${TOR_ARCHIVE_HASH_ANDROID} \
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
# ===== End of libxz functions ===============================================

# ============================================================================

# ===== Start of NDK functions ===============================================
checkNDKArchive() {
    info ""
    info "NDK:"
    info " -> Searching Android toolchain file ${ANDROID_TOOLCHAIN_CMAKE}"
    if [[ -e ${ANDROID_TOOLCHAIN_CMAKE} ]]; then
        info " -> Found it! :-)"
    else
        warning " -> Android toolchain file ${ANDROID_TOOLCHAIN_CMAKE} not found!"
        local currentDir=$(pwd)
        if [[ ! -e ${ANDROID_NDK_ARCHIVE_LOCATION} ]]; then
            mkdir -p ${ANDROID_NDK_ARCHIVE_LOCATION}
        fi
        cd ${ANDROID_NDK_ARCHIVE_LOCATION}
        if [[ -e "${ANDROID_NDK_ARCHIVE}" ]]; then
            info " -> Using existing NDK archive"
        else
            ANDROID_NDK_ARCHIVE_URL=https://dl.google.com/android/repository/${ANDROID_NDK_ARCHIVE}
            info " -> Downloading NDK archive ${ANDROID_NDK_ARCHIVE_URL}"
            wget ${ANDROID_NDK_ARCHIVE_URL}
        fi
        info " -> Cleanup before extraction"
        rm -rf android-ndk-${ANDROID_NDK_VERSION}
        info " -> Extracting NDK archive"
        unzip ${ANDROID_NDK_ARCHIVE}
        cd - >/dev/null
    fi
}
# ===== End of NDK functions =================================================

# ============================================================================

# Determine system
# Determine amount of cores:
if [[ "$OSTYPE" == "linux-gnu" ]]; then
    CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
    ANDROID_NDK_ARCHIVE=android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.zip
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    CORES_TO_USE=$(system_profiler SPHardwareDataType | grep "Total Number of Cores" | tr -s " " | cut -d " " -f 6)
    ANDROID_NDK_ARCHIVE=android-ndk-${ANDROID_NDK_VERSION}-darwin-x86_64.zip
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

while getopts a:c:dfgop:th? option; do
    case ${option} in
    a) ANDROID_TOOLCHAIN_CMAKE="${OPTARG}" ;;
    c) CORES_TO_USE="${OPTARG}" ;;
    d) BUILD_ONLY_DEPENDENCIES=true ;;
    f) FULLBUILD=true ;;
    g)
        ENABLE_GUI=true
        ENABLE_GUI_PARAMETERS="ON -DQT_CMAKE_MODULE_PATH=${ANDROID_QT_LIBRARYDIR}/cmake"
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
checkNDKArchive
checkBoost
checkBerkeleyDB
checkLevelDB
checkOpenSSL
if ${WITH_TOR}; then
    checkXZLib
    checkZStdLib
    checkEventLib
    checkTor
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
    -DANDROID=1 \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH=${ANDROID_ARCH} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    \
    -DENABLE_GUI=${ENABLE_GUI_PARAMETERS} \
    \
    -DBOOST_ROOT=${BOOST_ROOT} \
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR} \
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR} \
    \
    -DBerkeleyDB_ROOT_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install \
    -DBERKELEYDB_INCLUDE_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install/include \
    \
    -Dleveldb_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/cmake/leveldb \
    \
    -DOPENSSL_ROOT_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib;${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/include
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
    error " => Finished with return code ${rtc}"
fi
cd "${callDir}" || die 1 "Unable to cd back to where we came from (${callDir})"
