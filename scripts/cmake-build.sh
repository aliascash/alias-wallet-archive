#!/bin/bash
# ===========================================================================
#
# Created: 2019-10-10 HLXEasy
#
# This script can be used to build Spectrecoin using CMake
#
# ===========================================================================

BUILD_DIR=cmake-build-cmdline

# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${ownLocation}" || die 1 "Unable to cd into own location ${ownLocation}"
. ./include/helpers_console.sh

_init

# Determine amount of cores:
CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
FULLBUILD=false

while getopts c:fh? option; do
    case ${option} in
        c) CORES_TO_USE="${OPTARG}";;
        f) FULLBUILD=true;;
        h|?) helpMe && exit 0;;
        *) die 90 "invalid option \"${OPTARG}\"";;
    esac
done

# Go to Spectrecoin repository root directory
cd ..

if [[ ! -d ${BUILD_DIR} ]] ; then
    info "Creating build directory ${BUILD_DIR}"
    mkdir ${BUILD_DIR}
fi

cd ${BUILD_DIR} || die 1 "Unable to cd into ${BUILD_DIR}"

if ${FULLBUILD} ; then
    info "Cleanup leftovers from previous build run"
    rm -rf ./*

fi

info "Generating build configuration"
cmake \
    -DBUILD_OPENSSL=ON \
    -DOPENSSL_BUILD_VERSION=1.1.0l \
    ..

info "Building with ${CORES_TO_USE} cores:"
cmake \
    --build . \
    -- \
    -j "${CORES_TO_USE}"

rtc=$?
if [[ $rtc = 0 ]] ; then
    info "Finished"
else
    error "Finished with return code ${rtc}"
fi
cd "${callDir}" || die 1 "Unable to cd back to where we came from (${callDir})"
