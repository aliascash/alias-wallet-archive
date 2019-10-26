#!/bin/bash
# ===========================================================================
#
# Created: 2019-10-10 HLXEasy
#
# This script can be used to build Spectrecoin using CMake
#
# ===========================================================================

BUILD_DIR=cmake-build-cmdline

# Location of archive will be resolved like this:
# ${BERKELEYDB_ARCHIVE_LOCATION}/db-${BERKELEYDB_BUILD_VERSION}.zip
BERKELEYDB_ARCHIVE_LOCATION=~/BerkeleyDB
#BERKELEYDB_BUILD_VERSION=4.8.30
BERKELEYDB_BUILD_VERSION=5.0.32

# Location of archive will be resolved like this:
# ${OPENSSL_ARCHIVE_LOCATION}/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
#OPENSSL_ARCHIVE_LOCATION=https://mirror.viaduck.org/openssl
OPENSSL_ARCHIVE_LOCATION=~/OpenSSL
#OPENSSL_BUILD_VERSION=1.1.0l
OPENSSL_BUILD_VERSION=1.1.1d

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

_init

# Determine amount of cores:
CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
FULLBUILD=false

while getopts b:c:fo:h? option; do
    case ${option} in
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

info ""
info "Generating build configuration"
read -r -d '' cmd << EOM
cmake \
    -DBUILD_OPENSSL=ON \
    -DOPENSSL_ARCHIVE_LOCATION=${OPENSSL_ARCHIVE_LOCATION} \
    -DOPENSSL_BUILD_VERSION=${OPENSSL_BUILD_VERSION} \
    -DOPENSSL_API_COMPAT=0x00908000L \
    -DBERKELEYDB_ARCHIVE_LOCATION=${BERKELEYDB_ARCHIVE_LOCATION} \
    -DBERKELEYDB_BUILD_VERSION=${BERKELEYDB_BUILD_VERSION} \
    -DBERKELEYDB_BUILD_VERSION_SHORT=${BERKELEYDB_BUILD_VERSION%.*} \
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
