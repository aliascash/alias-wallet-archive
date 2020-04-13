#!/bin/bash
# ===========================================================================
#
# Created: 2019-10-10 HLXEasy
#
# This script can be used to build Spectrecoin using CMake
#
# ===========================================================================

# ===========================================================================
# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${ownLocation}" || die 1 "Unable to cd into own location ${ownLocation}"
. ./include/helpers_console.sh
_init
. ./include/handle_buildconfig.sh

##### ### # Global definitions # ### ########################################
##### ### # Boost # ### #####################################################
# Location of Boost will be resolved by trying to find required Boost libs
BOOST_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Boost
BOOST_INCLUDEDIR=${BOOST_ARCHIVE_LOCATION}/boost_${BOOST_VERSION//./_}
BOOST_LIBRARYDIR=${BOOST_ARCHIVE_LOCATION}/boost_${BOOST_VERSION//./_}/stage/lib
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

BUILD_DIR=cmake-build-cmdline_x86

helpMe() {
    echo "

    Helper script to build Spectrecoin wallet and daemon using CMake.
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
    -f  Perform fullbuild by cleanup all generated data from previous
        build runs.
    -g  Build UI (Qt) components.
    -s  Perfom only Spectrecoin fullbuild. Only the spectrecoin buildfolder
        will be wiped out before. All other folders stay in place.
    -t  Build with included Tor
    -h  Show this help

    "
}

# ===== Start of berkeleydb functions ========================================
checkOpenSSLArchive(){
    if [[ -e "${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz" ]] ; then
        info " -> Using OpenSSL archive ${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz"
    else
        OPENSSL_ARCHIVE_URL=https://mirror.viaduck.org/openssl/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
        info " -> Downloading OpenSSL archive ${OPENSSL_ARCHIVE_URL}"
        if [[ ! -e ${OPENSSL_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${OPENSSL_ARCHIVE_LOCATION}
        fi
        cd ${OPENSSL_ARCHIVE_LOCATION}
        wget ${OPENSSL_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

# For OpenSSL we're using a fork of https://github.com/viaduck/openssl-cmake
# with some slight modifications for Spectrecoin
checkOpenSSLClone(){
    local currentDir=$(pwd)
    cd ${ownLocation}/../external
    if [[ -d openssl-cmake ]] ; then
        info " -> Updating openssl-cmake clone"
        cd openssl-cmake
        git pull --prune
    else
        info " -> Cloning openssl-cmake"
        git clone --branch spectrecoin https://github.com/spectrecoin/openssl-cmake.git openssl-cmake
    fi
    cd "${currentDir}"
}

checkOpenSSLBuild(){
    mkdir -p ${BUILD_DIR}/openssl
    cd ${BUILD_DIR}/openssl

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DBUILD_OPENSSL=ON \
    -DOPENSSL_ARCHIVE_LOCATION=${OPENSSL_ARCHIVE_LOCATION} \
    -DOPENSSL_BUILD_VERSION=${OPENSSL_BUILD_VERSION} \
    -DOPENSSL_API_COMPAT=0x00908000L \
    -DOPENSSL_ARCHIVE_HASH=${OPENSSL_ARCHIVE_HASH} \
    ${BUILD_DIR}/../external/openssl-cmake
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished openssl build and install"
    else
        error " => Finished openssl with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkOpenSSL(){
    info ""
    info "OpenSSL:"
    if [[ -f ${BUILD_DIR}/usr/local/lib/libssl.a ]] ; then
        info " -> Found ${BUILD_DIR}/usr/local/lib/libssl.a, skip build"
    else
        checkOpenSSLArchive
        checkOpenSSLClone
        checkOpenSSLBuild
    fi
}
# ===== End of openssl functions =============================================

# ============================================================================

# ===== Start of berkeleydb functions ========================================
checkBerkeleyDBArchive(){
    if [[ -e "${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz" ]] ; then
        info " -> Using BerkeleyDB archive ${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz"
    else
        BERKELEYDB_ARCHIVE_URL=https://download.oracle.com/berkeley-db/db-${BERKELEYDB_BUILD_VERSION}.tar.gz
        info " -> Downloading BerkeleyDB archive ${BERKELEYDB_ARCHIVE_URL}"
        if [[ ! -e ${BERKELEYDB_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${BERKELEYDB_ARCHIVE_LOCATION}
        fi
        cd ${BERKELEYDB_ARCHIVE_LOCATION}
        wget ${BERKELEYDB_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkBerkeleyDBBuild(){
    mkdir -p ${BUILD_DIR}/libdb
    cd ${BUILD_DIR}/libdb

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DBERKELEYDB_ARCHIVE_LOCATION=${BERKELEYDB_ARCHIVE_LOCATION} \
    -DBERKELEYDB_BUILD_VERSION=${BERKELEYDB_BUILD_VERSION} \
    -DBERKELEYDB_BUILD_VERSION_SHORT=${BERKELEYDB_BUILD_VERSION%.*} \
    -DBERKELEYDB_ARCHIVE_HASH=${BERKELEYDB_ARCHIVE_HASH} \
    -DCMAKE_INSTALL_PREFIX=/libdb-install \
    ${BUILD_DIR}/../external/berkeleydb-cmake
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished libdb build and install"
    else
        error " => Finished libdb with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkBerkeleyDB(){
    info ""
    info "BerkeleyDB:"
    if [[ -f ${BUILD_DIR}/libdb/libdb-install/lib/libdb.a ]] ; then
        info " -> Found ${BUILD_DIR}/libdb/libdb-install/lib/libdb.a, skip build"
    else
        checkBerkeleyDBArchive
        checkBerkeleyDBBuild
    fi
}
# ===== End of berkeleydb functions ==========================================

# ============================================================================

# ===== Start of boost functions =============================================
checkBoost(){
    info ""
    info "Boost:"
    info " -> Searching required static Boost libs"
    buildBoost=false
    for currentBoostDependency in ${BOOST_REQUIRED_LIBS} ; do
        if [[ -e ${BOOST_LIBRARYDIR}/libboost_${currentBoostDependency}.a ]] ; then
            info " -> ${currentBoostDependency}: OK"
        else
            warning " => ${currentBoostDependency}: Not found!"
            buildBoost=true
        fi
    done
    if ${buildBoost} ; then
        local currentDir=$(pwd)
        if [[ ! -e ${BOOST_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${BOOST_ARCHIVE_LOCATION}
        fi
        cd ${BOOST_ARCHIVE_LOCATION}
        if [[ ! -e "boost_${BOOST_VERSION//./_}.tar.gz" ]] ; then
            info " -> Downloading Boost archive"
            wget https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
        else
            info " -> Using existing Boost archive"
        fi
        info " -> Cleanup before extraction"
        rm -rf boost_${BOOST_VERSION//./_}
        info " -> Extracting Boost archive"
        tar xzf boost_${BOOST_VERSION//./_}.tar.gz
        info " -> Building Boost"
        cd boost_${BOOST_VERSION//./_}
        ./bootstrap.sh --with-libraries="${BOOST_REQUIRED_LIBS// /,}"
#        ./bootstrap.sh
        ./b2 address-model=32 -j"${CORES_TO_USE}"
        cd "${currentDir}"
    fi
}
# ===== End of boost functions ===============================================

# ============================================================================

# ===== Start of libevent functions ==========================================
checkEventLibArchive(){
    if [[ -e "${LIBEVENT_ARCHIVE_LOCATION}/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz" ]] ; then
        info " -> Using EventLib archive ${LIBEVENT_ARCHIVE_LOCATION}/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz"
    else
        LIBEVENT_ARCHIVE_URL=https://github.com/libevent/libevent/releases/download/release-${LIBEVENT_BUILD_VERSION}-stable/libevent-${LIBEVENT_BUILD_VERSION}-stable.tar.gz
        info " -> Downloading EventLib archive ${LIBEVENT_ARCHIVE_URL}"
        if [[ ! -e ${LIBEVENT_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${LIBEVENT_ARCHIVE_LOCATION}
        fi
        cd ${LIBEVENT_ARCHIVE_LOCATION}
        wget ${LIBEVENT_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkEventLibClone(){
    local currentDir=$(pwd)
    cd ${ownLocation}/../external
    if [[ -d libevent ]] ; then
        info " -> Updating libevent clone"
        cd libevent
        git pull --prune
    else
        info " -> Cloning libevent"
        git clone https://github.com/libevent/libevent.git libevent
    fi
    cd "${currentDir}"
}

checkEventLibBuild(){
    mkdir -p ${BUILD_DIR}/libevent
    cd ${BUILD_DIR}/libevent

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DOPENSSL_ROOT_DIR=${BUILD_DIR}/usr/local/lib;${BUILD_DIR}/usr/local/include \
    \
    -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/usr/local \
    ${BUILD_DIR}/../external/libevent
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished libevent build, installing..."
        make install || error "Error during installation of libevent"
    else
        error " => Finished libevent with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkEventLib(){
    info ""
    info "EventLib:"
    if [[ -f ${BUILD_DIR}/usr/local/lib/libevent.a ]] ; then
        info " -> Found ${BUILD_DIR}/usr/local/lib/libevent.a, skip build"
    else
        checkEventLibClone
        checkEventLibBuild
    fi
}

# ===== End of libevent functions ============================================

# ============================================================================

# ===== Start of leveldb functions ===========================================
checkLevelDBClone(){
    local currentDir=$(pwd)
    cd ${ownLocation}/../external
    if [[ -d leveldb ]] ; then
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

checkLevelDBBuild(){
    mkdir -p ${BUILD_DIR}/libleveldb
    cd ${BUILD_DIR}/libleveldb

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/usr/local \
    ${BUILD_DIR}/../external/leveldb
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished libevent build, installing..."
        make install || error "Error during installation of libevent"
    else
        error " => Finished libevent with return code ${rtc}"
    fi
#    read a
    cd - >/dev/null
}

checkLevelDB(){
    info ""
    info "LevelDB:"
    if [[ -f ${BUILD_DIR}/usr/local/lib/libleveldb.a ]] ; then
        info " -> Found ${BUILD_DIR}/usr/local/lib/libleveldb.a, skip build"
    else
        checkLevelDBClone
        checkLevelDBBuild
    fi
}

# ===== End of leveldb functions =============================================

# ============================================================================

# ===== Start of libzstd functions ===========================================
checkZStdLibArchive(){
    if [[ -e "${LIBZ_ARCHIVE_LOCATION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz" ]] ; then
        info " -> Using ZLib archive ${LIBZ_ARCHIVE_LOCATION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz"
    else
        LIBZ_ARCHIVE_URL=https://github.com/facebook/zstd/releases/download/v${LIBZ_BUILD_VERSION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz
        info " -> Downloading ZLib archive ${LIBZ_ARCHIVE_URL}"
        if [[ ! -e ${LIBZ_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${LIBZ_ARCHIVE_LOCATION}
        fi
        cd ${LIBZ_ARCHIVE_LOCATION}
        wget ${LIBZ_ARCHIVE_URL}
        cd - >/dev/null
    fi
    cd ${ownLocation}/../external
    if [[ -d libzstd ]] ; then
        info " -> Directory external/libzstd already existing. Remove it to extract it again"
    else
        info " -> Extracting zstd-${LIBZ_BUILD_VERSION}.tar.gz..."
        tar xzf ${LIBZ_ARCHIVE_LOCATION}/zstd-${LIBZ_BUILD_VERSION}.tar.gz
        mv zstd-${LIBZ_BUILD_VERSION} libzstd
    fi
    cd - >/dev/null
}

checkZStdLibBuild(){
    mkdir -p ${BUILD_DIR}/libzstd
    cd ${BUILD_DIR}/libzstd

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DLIBZ_ARCHIVE_LOCATION=${LIBZ_ARCHIVE_LOCATION} \
    -DLIBZ_BUILD_VERSION=${LIBZ_BUILD_VERSION} \
    -DLIBZ_BUILD_VERSION_SHORT=${LIBZ_BUILD_VERSION%.*} \
    -DLIBZ_ARCHIVE_HASH=${LIBZ_ARCHIVE_HASH} \
    -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/usr/local \
    ${BUILD_DIR}/../external/libzstd/build/cmake
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished libzstd build, installing..."
        make install || error "Error during installation of libzstd"
    else
        error " => Finished libzstd with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkZStdLib(){
    info ""
    info "ZStdLib:"
    if [[ -f ${BUILD_DIR}/usr/local/lib/libzstd.a ]] ; then
        info " -> Found ${BUILD_DIR}/usr/local/lib/libzstd.a, skip build"
    else
        checkZStdLibArchive
        checkZStdLibBuild
    fi
}
# ===== End of libzstd functions =============================================

# ============================================================================

# ===== Start of libxz functions =============================================
checkXZLibArchive(){
    if [[ -e "${LIBXZ_ARCHIVE_LOCATION}/xz-${LIBXZ_BUILD_VERSION}.tar.gz" ]] ; then
        info " -> Using XZLib archive ${LIBXZ_ARCHIVE_LOCATION}/xz-${LIBXZ_BUILD_VERSION}.tar.gz"
    else
        LIBXZ_ARCHIVE_URL=https://tukaani.org/xz/xz-${LIBXZ_BUILD_VERSION}.tar.gz
        info " -> Downloading XZLib archive ${LIBZ_ARCHIVE_URL}"
        if [[ ! -e ${LIBXZ_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${LIBXZ_ARCHIVE_LOCATION}
        fi
        cd ${LIBXZ_ARCHIVE_LOCATION}
        wget ${LIBXZ_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkXZLibBuild(){
    mkdir -p ${BUILD_DIR}/libxz
    cd ${BUILD_DIR}/libxz

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DLIBXZ_ARCHIVE_LOCATION=${LIBXZ_ARCHIVE_LOCATION} \
    -DLIBXZ_BUILD_VERSION=${LIBXZ_BUILD_VERSION} \
    -DLIBXZ_BUILD_VERSION_SHORT=${LIBXZ_BUILD_VERSION%.*} \
    -DLIBXZ_ARCHIVE_HASH=${LIBXZ_ARCHIVE_HASH} \
    ${BUILD_DIR}/../external/libxz-cmake
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished libxz build and install"
    else
        error " => Finished libxz with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkXZLib(){
    info ""
    info "XZLib:"
    if [[ -f ${BUILD_DIR}/usr/local/lib/liblzma.a ]] ; then
        info " -> Found ${BUILD_DIR}/usr/local/lib/liblzma.a, skip build"
    else
        checkXZLibArchive
        checkXZLibBuild
    fi
}
# ===== End of libxz functions ===============================================

# ============================================================================

# ===== Start of tor functions ===============================================
checkTorArchive(){
    if [[ -e "${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION}.tar.gz" ]] ; then
        info " -> Using Tor archive ${TOR_ARCHIVE_LOCATION}/tor-${TOR_BUILD_VERSION}.tar.gz"
    else
        TOR_ARCHIVE_URL=https://github.com/torproject/tor/archive/tor-${TOR_BUILD_VERSION}.tar.gz
        info " -> Downloading Tor archive ${TOR_ARCHIVE_URL}"
        if [[ ! -e ${TOR_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${TOR_ARCHIVE_LOCATION}
        fi
        cd ${TOR_ARCHIVE_LOCATION}
        wget ${TOR_ARCHIVE_URL}
        cd - >/dev/null
    fi
}

checkTorBuild(){
    mkdir -p ${BUILD_DIR}/tor
    cd ${BUILD_DIR}/tor

    info " -> Generating build configuration"
    read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DTOR_ARCHIVE_LOCATION=${TOR_ARCHIVE_LOCATION} \
    -DTOR_BUILD_VERSION=${TOR_BUILD_VERSION} \
    -DTOR_BUILD_VERSION_SHORT=${TOR_BUILD_VERSION%.*} \
    -DTOR_ARCHIVE_HASH=${TOR_ARCHIVE_HASH} \
    ${BUILD_DIR}/../external/tor-cmake
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
    if [[ ${rtc} = 0 ]] ; then
        info " -> Finished tor build and install"
    else
        error " => Finished tor with return code ${rtc}"
    fi

    cd - >/dev/null
}

checkTor(){
    info ""
    info "Tor:"
    if [[ -f ${BUILD_DIR}/usr/local/bin/tor ]] ; then
        info " -> Found ${BUILD_DIR}/usr/local/bin/tor, skip build"
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

FULLBUILD=false
ENABLE_GUI=false
ENABLE_GUI_PARAMETERS='OFF'
BUILD_ONLY_SPECTRECOIN=false
WITH_TOR=false

while getopts c:fgsth? option; do
    case ${option} in
        c) CORES_TO_USE="${OPTARG}";;
        f) FULLBUILD=true;;
        g) ENABLE_GUI=true
           ENABLE_GUI_PARAMETERS="ON";;
        s) BUILD_ONLY_SPECTRECOIN=true;;
        t) WITH_TOR=true;;
        h|?) helpMe && exit 0;;
        *) die 90 "invalid option \"${OPTARG}\"";;
    esac
done

# Go to Spectrecoin repository root directory
cd ..

if [[ ! -d ${BUILD_DIR} ]] ; then
    info ""
    info "Creating build directory ${BUILD_DIR}"
    mkdir ${BUILD_DIR}
    info " -> Done"
fi

cd ${BUILD_DIR} || die 1 "Unable to cd into ${BUILD_DIR}"
BUILD_DIR=$(pwd)

if ${FULLBUILD} ; then
    info ""
    info "Cleanup leftovers from previous build run"
    rm -rf ./*
    info " -> Done"
elif ${BUILD_ONLY_SPECTRECOIN} ; then
    info ""
    info "Cleanup spectrecoin folder from previous build run"
    rm -rf ./spectrecoin
    info " -> Done"
fi

checkBoost
checkBerkeleyDB
checkLevelDB
checkOpenSSL
if ${WITH_TOR} ; then
    checkEventLib
    checkXZLib
    checkZStdLib
    checkTor
fi

mkdir -p ${BUILD_DIR}/spectrecoin
cd ${BUILD_DIR}/spectrecoin

info ""
info "Generating Spectrecoin build configuration"

# FindBerkeleyDB.cmake requires this
export BERKELEYDB_ROOT=${BUILD_DIR}/libdb/libdb-install

read -r -d '' cmd << EOM
cmake \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-m32 \
    -DCMAKE_MODULE_LINKER_FLAGS_INIT=-m32 \
    \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    \
    -DENABLE_GUI=${ENABLE_GUI_PARAMETERS} \
    \
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR} \
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR} \
    \
    -DBerkeleyDB_ROOT_DIR=${BUILD_DIR}/libdb/libdb-install \
    -DBERKELEYDB_INCLUDE_DIR=${BUILD_DIR}/libdb/libdb-install/include \
    \
    -Dleveldb_DIR=${BUILD_DIR}/usr/local/lib/cmake/leveldb \
    \
    -DOPENSSL_ROOT_DIR=${BUILD_DIR}/usr/local/lib;${BUILD_DIR}/usr/local/include
EOM
if ${WITH_TOR} ; then
    read -r -d '' cmd << EOM
${cmd} \
    -DWITH_TOR=ON \
    ${BUILD_DIR}/..
EOM
else
    read -r -d '' cmd << EOM
${cmd} \
    ${BUILD_DIR}/..
EOM
fi

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
if [[ ${rtc} = 0 ]] ; then
    info " -> Finished"
else
    error " => Finished with return code ${rtc}"
fi
cd "${callDir}" || die 1 "Unable to cd back to where we came from (${callDir})"
