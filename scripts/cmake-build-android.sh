#!/bin/bash
# ===========================================================================
#
# Created: 2019-10-22 HLXEasy
#
# This script can be used to build Spectrecoin for Android using CMake
#
# ===========================================================================

BUILD_DIR=cmake-build-android-cmdline

##### ### # Android # ### ###################################################
ANDROID_NDK_ROOT=/home/spectre/Android/ndk/android-ndk-r20
ANDROID_TOOLCHAIN_CMAKE=${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake
ANDROID_ARCH=arm64
ANDROID_ABI=arm64-v8a
ANDROID_API=23

##### ### # Boost # ### #####################################################
# Location of Boost will be resolved by trying to find required Boost libs
BOOST_VERSION=1.68.0
BOOST_DIR=~/Boost
BOOST_ROOT=${BOOST_DIR}/boost_${BOOST_VERSION//./_}_android_arm64
BOOST_INCLUDEDIR=${BOOST_ROOT}/include
BOOST_LIBRARYDIR=${BOOST_ROOT}/lib
BOOST_REQUIRED_LIBS='chrono filesystem iostreams program_options system thread regex date_time atomic'
# regex date_time atomic

##### ### # BerkeleyDB # ### ################################################
# Location of archive will be resolved like this:
# ${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.zip
BERKELEYDB_ARCHIVE_LOCATION=~/BerkeleyDB
BERKELEYDB_BUILD_VERSION=4.8.30
#BERKELEYDB_BUILD_VERSION=5.0.32
#BERKELEYDB_BUILD_VERSION=6.2.38

##### ### # OpenSSL # ### ###################################################
# Location of archive will be resolved like this:
# ${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
#OPENSSL_ARCHIVE_LOCATION=https://mirror.viaduck.org/openssl
OPENSSL_ARCHIVE_LOCATION=~/OpenSSL
OPENSSL_BUILD_VERSION=1.1.0l
#OPENSSL_BUILD_VERSION=1.1.1d

# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${ownLocation}" || die 1 "Unable to cd into own location ${ownLocation}"
. ./include/helpers_console.sh

helpMe() {
    echo "

    Helper script to build Spectrecoin wallet and daemon using CMake.
    Assumptions:
    - The BerkeleyDB archive to use must be located on ${BERKELEYDB_ARCHIVE_LOCATION}
    - Naming must be default like 'db-4.8.30.zip'
    - Android toolchain already installed

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
    -b <version>
        BerkeleyDB version to use. Corresponding archive must be located
        on ${BERKELEYDB_ARCHIVE_LOCATION}. Default: ${BERKELEYDB_BUILD_VERSION}
    -o <version>
        OpenSSL version to use. Corresponding version will be downloaded
        automatically. Default: ${OPENSSL_BUILD_VERSION}
    -h  Show this help

    "
}

checkOpenSSLArchive(){
    info ""
    if [[ -e "${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz" ]] ; then
        info "Using OpenSSL archive ${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz"
    else
        OPENSSL_ARCHIVE_URL=https://mirror.viaduck.org/openssl/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
        info "Downloading OpenSSL archive ${OPENSSL_ARCHIVE_URL}"
        if [[ ! -e ${OPENSSL_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${OPENSSL_ARCHIVE_LOCATION}
        fi
        cd ${OPENSSL_ARCHIVE_LOCATION}
        wget ${OPENSSL_ARCHIVE_URL}
        cd -
    fi
}

checkBerkeleyDBArchive(){
    info ""
    if [[ -e "${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz" ]] ; then
        info "Using BerkeleyDB archive ${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.tar.gz"
    else
        BERKELEYDB_ARCHIVE_URL=https://download.oracle.com/berkeley-db/db-${BERKELEYDB_BUILD_VERSION}.tar.gz
        info "Downloading BerkeleyDB archive ${BERKELEYDB_ARCHIVE_URL}"
        if [[ ! -e ${BERKELEYDB_ARCHIVE_LOCATION} ]] ; then
            mkdir -p ${BERKELEYDB_ARCHIVE_LOCATION}
        fi
        cd ${BERKELEYDB_ARCHIVE_LOCATION}
        wget ${BERKELEYDB_ARCHIVE_URL}
        cd -
    fi
}

disableUserConfig(){
    if [[ -e ~/user-config.jam ]] ; then
        mv ~/user-config.jam ~/user-config.jam.disabled
    fi
}

enableUserConfig(){
    if [[ -e ~/user-config.jam.disabled ]] ; then
        mv ~/user-config.jam.disabled ~/user-config.jam
    fi
}

checkBoost(){
    info ""
    info "Searching required static Boost libs"
    buildBoost=false
    if [[ -d ${BOOST_LIBRARYDIR} ]] ; then
        for currentBoostDependency in ${BOOST_REQUIRED_LIBS} ; do
            if [[ -n $(find ${BOOST_LIBRARYDIR}/ -name "libboost_${currentBoostDependency}*.a") ]] ; then
                info "${currentBoostDependency}: OK"
            else
                warning "${currentBoostDependency}: Not found!"
                buildBoost=true
            fi
        done
    else
        warning "Boost library directory ${BOOST_LIBRARYDIR} not found!"
        buildBoost=true
    fi
    if ${buildBoost} ; then
        local currentDir=$(pwd)
        cd ${BOOST_DIR}
        if [[ ! -e "boost_${BOOST_VERSION//./_}.tar.gz" ]] ; then
            info "Downloading and extracting Boost archive"
            wget https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
        else
            info "Using existing Boost archive"
        fi
        rm -rf boost_${BOOST_VERSION//./_}_android
        mkdir boost_${BOOST_VERSION//./_}_android
        cd boost_${BOOST_VERSION//./_}_android
        tar xzf ../boost_${BOOST_VERSION//./_}.tar.gz
        cd boost_${BOOST_VERSION//./_}
        mv * ../
        cd - >/dev/null
        rm -rf boost_${BOOST_VERSION//./_}
        info "Building Boost"

        enableUserConfig
        ${ownLocation}/build-boost-for-android.sh -v ${BOOST_VERSION} -a ${ANDROID_ARCH} -n ${ANDROID_NDK_ROOT} -l ${BOOST_REQUIRED_LIBS// /,}
        disableUserConfig

        cd "${currentDir}"
    fi
}

_init

# Determine amount of cores:
CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
FULLBUILD=false

while getopts a:b:c:fo:h? option; do
    case ${option} in
        a) ANDROID_TOOLCHAIN_CMAKE="${OPTARG}";;
        b) BERKELEYDB_BUILD_VERSION="${OPTARG}";;
        c) CORES_TO_USE="${OPTARG}";;
        f) FULLBUILD=true;;
        o) OPENSSL_BUILD_VERSION="${OPTARG}";;
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
fi

cd ${BUILD_DIR} || die 1 "Unable to cd into ${BUILD_DIR}"

if ${FULLBUILD} ; then
    info ""
    info "Cleanup leftovers from previous build run"
    rm -rf ./*
fi

checkBerkeleyDBArchive
checkOpenSSLArchive
checkBoost

info ""
info "Generating build configuration"
read -r -d '' cmd << EOM
cmake \
    -DANDROID=1 \
    -DCMAKE_ANDROID_API=${ANDROID_API} \
    -DANDROID_PLATFORM=${ANDROID_API} \
    -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ARCH}-v8a \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT} \
    -DANDROID_ABI=${ANDROID_ABI} \
    \
    -DBERKELEYDB_ARCHIVE_LOCATION=${BERKELEYDB_ARCHIVE_LOCATION} \
    -DBERKELEYDB_BUILD_VERSION=${BERKELEYDB_BUILD_VERSION} \
    -DBERKELEYDB_BUILD_VERSION_SHORT=${BERKELEYDB_BUILD_VERSION%.*} \
    \
    -DBOOST_ROOT=${BOOST_ROOT} \
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR} \
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR} \
    -DBoost_INCLUDE_DIR=${BOOST_INCLUDEDIR} \
    -DBoost_LIBRARY_DIR=${BOOST_LIBRARYDIR} \
    \
    -DBUILD_OPENSSL=ON \
    -DOPENSSL_ARCHIVE_LOCATION=${OPENSSL_ARCHIVE_LOCATION} \
    -DOPENSSL_API_COMPAT=0x00908000L \
    -DOPENSSL_BUILD_VERSION=${OPENSSL_BUILD_VERSION} \
    -DCROSS_ANDROID=ON \
    ..
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
cmake \
    --build . \
    -- \
    -j "${CORES_TO_USE}"

rtc=$?
info ""
if [[ ${rtc} = 0 ]] ; then
    info "Finished"
else
    error "Finished with return code ${rtc}"
fi
cd "${callDir}" || die 1 "Unable to cd back to where we came from (${callDir})"
