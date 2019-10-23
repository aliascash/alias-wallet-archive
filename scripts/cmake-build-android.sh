#!/bin/bash
# ===========================================================================
#
# Created: 2019-10-22 HLXEasy
#
# This script can be used to build Spectrecoin for Android using CMake
#
# ===========================================================================

BUILD_DIR=cmake-build-android-cmdline

MY_BOOST_LIBS_DIR=/home/spectre/coding/Boost-for-Android/build/out

BERKELEYDB_ARCHIVE_LOCATION=~/BerkeleyDB
BERKELEYDB_VERSION=4.8.30
OPENSSL_VERSION=1.1.0l
ANDROID_TOOLCHAIN_CMAKE=/home/spectre/Android/ndk/20.0.5594570/build/cmake/android.toolchain.cmake
ANDROID_ABI=arm64-v8a
ANDROID_NDK_ROOT=/home/spectre/Android/ndk/20.0.5594570

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
        on ${BERKELEYDB_ARCHIVE_LOCATION}. Default: ${BERKELEYDB_VERSION}
    -o <version>
        OpenSSL version to use. Corresponding version will be downloaded
        automatically. Default: ${OPENSSL_VERSION}
    -h  Show this help

    "
}

_init

# Determine amount of cores:
CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
FULLBUILD=false

while getopts a:b:c:fo:h? option; do
    case ${option} in
        a) ANDROID_TOOLCHAIN_CMAKE="${OPTARG}";;
        b) BERKELEYDB_VERSION="${OPTARG}";;
        c) CORES_TO_USE="${OPTARG}";;
        f) FULLBUILD=true;;
        o) OPENSSL_VERSION="${OPTARG}";;
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

info ""
info "Generating build configuration"
read -r -d '' cmd << EOM
cmake \
    -DANDROID=1 \
    \
    -DBERKELEYDB_ARCHIVE_LOCATION=${BERKELEYDB_ARCHIVE_LOCATION} \
    -DBERKELEYDB_BUILD_VERSION=${BERKELEYDB_VERSION} \
    -DBERKELEYDB_BUILD_VERSION_SHORT=${BERKELEYDB_VERSION%.*} \
    \
    -DMY_BOOST_LIBS_DIR=${MY_BOOST_LIBS_DIR} \
    \
    -DBUILD_OPENSSL=ON \
    -DOPENSSL_API_COMPAT=0x00908000L \
    -DOPENSSL_BUILD_VERSION=${OPENSSL_VERSION} \
    -DCROSS_ANDROID=ON \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_CMAKE} \
    -DANDROID_ABI=${ANDROID_ABI} \
    -DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT} \
    ..
EOM

echo "=============================================================================="
echo "Executing the following CMake cmd:"
echo "${cmd}"
echo "=============================================================================="
#read a
${cmd}

info ""
info "Building with ${CORES_TO_USE} cores:"
cmake \
    --build . \
    -- \
    -j "${CORES_TO_USE}"

rtc=$?
info ""
if [[ $rtc = 0 ]] ; then
    info "Finished"
else
    error "Finished with return code ${rtc}"
fi
cd "${callDir}" || die 1 "Unable to cd back to where we came from (${callDir})"
