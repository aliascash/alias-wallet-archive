#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2019-10-10 HLXEasy
#
# This script can be used to build Alias using CMake
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

##### ### # Boost # ### #####################################################
# Location of Boost will be resolved by trying to find required Boost libs
BOOST_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Boost
BOOST_REQUIRED_LIBS='chrono filesystem iostreams program_options system thread regex date_time atomic'
# regex date_time atomic

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

BUILD_DIR=cmake-build-cmdline

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

# ===== Determining used distribution ========================================
defineQtVersionForCurrentDistribution() {
    info ""
    info "Determining distribution:"
    if [[ -e /etc/os-release ]]; then
        . /etc/os-release
        usedDistro=''
        releaseName=''
        case ${ID} in
        "centos")
            usedDistro="CENTOS"
            case ${VERSION_ID} in
            "8")
                releaseName='8'
                ;;
            *)
                echo "Unsupported operating system ID=${ID}, VERSION_ID=${VERSION_ID}"
                ;;
            esac
            ;;
        "debian")
            usedDistro="DEBIAN"
            case ${VERSION_ID} in
            "9")
                releaseName='STRETCH'
                ;;
            "10")
                releaseName='BUSTER'
                ;;
            *)
                case ${PRETTY_NAME} in
                *"bullseye"*)
                    echo "Detected ${PRETTY_NAME}, installing Buster binaries"
                    releaseName='BUSTER'
                    ;;
                *)
                    echo "Unsupported operating system ID=${ID}, VERSION_ID=${VERSION_ID}"
                    cat /etc/os-release
                    exit 1
                    ;;
                esac
                ;;
            esac
            ;;
        "ubuntu")
            usedDistro="UBUNTU"
            case ${VERSION_CODENAME} in
            "bionic" | "cosmic")
                releaseName='1804'
                ;;
            "disco")
                releaseName='1904'
                ;;
            "eoan")
                releaseName='1910'
                ;;
            "focal")
                releaseName='2004'
                ;;
            *)
                echo "Unsupported operating system ID=${ID}, VERSION_ID=${VERSION_CODENAME}"
                exit
                ;;
            esac
            ;;
        "fedora")
            usedDistro="FEDORA"
            ;;
        "opensuse-tumbleweed")
            usedDistro="OPENSUSE"
            releaseName="TUMBLEWEED"
            ;;
        "raspbian")
            usedDistro="RASPBERRY"
            case ${VERSION_ID} in
            "9")
                releaseName='STRETCH'
                ;;
            "10")
                releaseName='BUSTER'
                ;;
            *)
                case ${PRETTY_NAME} in
                *"bullseye"*)
                    echo "Detected ${PRETTY_NAME}, installing Buster binaries"
                    releaseName='BUSTER'
                    ;;
                *)
                    echo "Unsupported operating system ID=${ID}, VERSION_ID=${VERSION_ID}"
                    cat /etc/os-release
                    exit 1
                    ;;
                esac
                ;;
            esac
            ;;
        *)
            echo "Unsupported operating system ${ID}, VERSION_ID=${VERSION_ID}"
            exit
            ;;
        esac

        # https://stackoverflow.com/questions/16553089/dynamic-variable-names-in-bash
        defineQtVersionToUse=QT_VERSION_TO_USE
        printf -v "$defineQtVersionToUse" '%s' "QT_VERSION_${usedDistro}_${releaseName}"

        # https://unix.stackexchange.com/questions/452723/is-it-possible-to-print-the-content-of-the-content-of-a-variable-with-shell-scri
        QT_VERSION="${!QT_VERSION_TO_USE}"
        QT_DIR=${QT_INSTALLATION_PATH}/${QT_VERSION}/gcc_64
        QT_LIBRARYDIR=${QT_DIR}/lib

        info " -> Determined ${usedDistro} ${releaseName}, using Qt ${QT_VERSION}"
    else
        die 100 "Unable to determine used Linux distribution"
    fi
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
    ./bootstrap.sh --with-libraries="${BOOST_REQUIRED_LIBS// /,}"
    #        ./bootstrap.sh
    ./b2 \
        -j"${CORES_TO_USE}" \
        --layout=tagged \
        --build-type=complete
    cd "${currentDir}" || die 1 "Unable to cd into ${currentDir}"
}

checkBoost() {
    info ""
    info "Boost:"
    info " -> Searching required static Boost libs"
    BOOST_INCLUDEDIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/boost_${BOOST_VERSION//./_}
    BOOST_LIBRARYDIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/boost_${BOOST_VERSION//./_}/stage/lib
    boostBuildRequired=false
    if [[ -d ${BOOST_LIBRARYDIR} ]]; then
        for currentBoostDependency in ${BOOST_REQUIRED_LIBS}; do
            if [[ -e ${BOOST_LIBRARYDIR}/libboost_${currentBoostDependency}-mt-${LIB_ARCH_SUFFIX}.a ]]; then
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
    if [[ -d ${QT_LIBRARYDIR} ]]; then
        # libQt5Quick.so
        for currentQtDependency in ${QT_REQUIRED_LIBS}; do
            if [[ -n $(find ${QT_LIBRARYDIR}/ -name "libQt5${currentQtDependency}.so") ]]; then
                info " -> ${currentQtDependency}: OK"
            else
                warning " -> ${currentQtDependency}: Not found!"
                qtComponentMissing=true
            fi
        done
    else
        info " -> Qt library directory ${QT_LIBRARYDIR} not found"
        qtComponentMissing=true
    fi
    if ${qtComponentMissing}; then
        error " -> Qt ${QT_VERSION}: Not all required components found!"
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

    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libleveldb.a ]]; then
        info " -> Using ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libleveldb.a"
        LIB_LEVELDB_LOCATION=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/cmake/leveldb
    elif [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib64/libleveldb.a ]]; then
        info " -> Using ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib64/libleveldb.a"
        LIB_LEVELDB_LOCATION=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib64/cmake/leveldb
    else
        die 23 " => libleveldb build result not found!"
    fi

    cd - >/dev/null
}

checkLevelDB() {
    info ""
    info "LevelDB:"
    if [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libleveldb.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/libleveldb.a, skip build"
        LIB_LEVELDB_LOCATION=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib/cmake/leveldb
    elif [[ -f ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib64/libleveldb.a ]]; then
        info " -> Found ${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib64/libleveldb.a, skip build"
        LIB_LEVELDB_LOCATION=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/usr/local/lib64/cmake/leveldb
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
# ===== End of libxz functions ===============================================

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
# Determine arch
LIB_ARCH_SUFFIX=x64
if [ "$(uname -m)" = "aarch64" ] ; then
    LIB_ARCH_SUFFIX=a64
fi

FULLBUILD=false
ENABLE_GUI=false
ENABLE_GUI_PARAMETERS='OFF'
BUILD_ONLY_ALIAS=false
BUILD_ONLY_DEPENDENCIES=false
WITH_TOR=false
SYSTEM_QT=false
LIB_LEVELDB_LOCATION=''
GIVEN_DEPENDENCIES_BUILD_DIR=''

defineQtVersionForCurrentDistribution

while getopts c:dfgop:sth? option; do
    case ${option} in
    c) CORES_TO_USE="${OPTARG}" ;;
    d) BUILD_ONLY_DEPENDENCIES=true ;;
    f) FULLBUILD=true ;;
    g) ENABLE_GUI=true ;;
    o) BUILD_ONLY_ALIAS=true ;;
    p) GIVEN_DEPENDENCIES_BUILD_DIR="${OPTARG}" ;;
    s) SYSTEM_QT=true ;;
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
# If GUI should be build, set proper parameters
if ${ENABLE_GUI}; then
    if ${SYSTEM_QT}; then
        ENABLE_GUI_PARAMETERS="ON"
    else
        checkQt
        ENABLE_GUI_PARAMETERS="ON -DQT_CMAKE_MODULE_PATH=${QT_LIBRARYDIR}/cmake"
    fi
fi

# ============================================================================
# Check a/o build requirements/dependencies
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

# FindBerkeleyDB.cmake requires this
export BERKELEYDB_ROOT=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install

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
    -DBerkeleyDB_ROOT_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install \
    -DBERKELEYDB_INCLUDE_DIR=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR}/libdb/libdb-install/include \
    \
    -Dleveldb_DIR=${LIB_LEVELDB_LOCATION} \
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
