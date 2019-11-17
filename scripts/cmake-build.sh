#!/bin/bash
# ===========================================================================
#
# Created: 2019-10-10 HLXEasy
#
# This script can be used to build Spectrecoin using CMake
#
# ===========================================================================

BUILD_DIR=cmake-build-cmdline

##### ### # Boost # ### #####################################################
# Location of Boost will be resolved by trying to find required Boost libs
BOOST_VERSION=1.69.0
BOOST_DIR=~/Boost
BOOST_INCLUDEDIR=${BOOST_DIR}/boost_1_69_0
BOOST_LIBRARYDIR=${BOOST_DIR}/boost_1_69_0/stage/lib
BOOST_REQUIRED_LIBS='chrono filesystem iostreams program_options system thread'

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

# ===========================================================================
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

checkBoost(){
    info ""
    info "Searching required static Boost libs"
    buildBoost=false
    for currentBoostDependency in ${BOOST_REQUIRED_LIBS} ; do
        if [[ -e ${BOOST_LIBRARYDIR}/libboost_${currentBoostDependency}.a ]] ; then
            info "${currentBoostDependency}: OK"
        else
            warning "${currentBoostDependency}: Not found!"
            buildBoost=true
        fi
    done
    if ${buildBoost} ; then
        local currentDir=$(pwd)
        cd ${BOOST_DIR}
        if [[ ! -e "boost_${BOOST_VERSION//./_}.tar.gz" ]] ; then
            info "Downloading and extracting Boost archive"
            wget https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
        else
            info "Using existing Boost archive"
        fi
        rm -rf boost_${BOOST_VERSION//./_}
        tar xzf boost_${BOOST_VERSION//./_}.tar.gz
        info "Building Boost"
        cd boost_${BOOST_VERSION//./_}
#        ./bootstrap.sh --with-libraries=${BOOST_REQUIRED_LIBS// /,}
        ./bootstrap.sh
        ./b2
        cd "${currentDir}"
    fi
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

checkBerkeleyDBArchive
checkOpenSSLArchive
checkBoost

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
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR} \
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR} \
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
