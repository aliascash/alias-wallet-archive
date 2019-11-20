#!/bin/bash

ANDROID_NDK_ROOT=/home/spectre/Android/ndk/android-ndk-r20
BOOST_VERSION=1.68.0
BOOST_LIBS_TO_BUILD=chrono
ANDROID_ARCH=arm64

# ===========================================================================
# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${ownLocation}" || die 1 "Unable to cd into own location ${ownLocation}"
. ./include/helpers_console.sh

helpMe() {
    echo "

    Helper script to build Boost for Android.

    Usage:
    ${0} [options]

    Optional parameters:
    -a <arch>
    -l <list-of-libs-to-builc>
    -n <path>
        Path to ndk directory.
    -v <version>
        Boost version to build. Default: ${BOOST_VERSION}
    -h  Show this help

    "
}

_init

while getopts a:l:n:v:h? option; do
    case ${option} in
        a) ANDROID_ARCH="${OPTARG}";;
        l) BOOST_LIBS_TO_BUILD="${OPTARG}";;
        n) ANDROID_NDK_ROOT="${OPTARG}";;
        v) BOOST_VERSION="${OPTARG}";;
        h|?) helpMe && exit 0;;
        *) die 90 "invalid option \"${OPTARG}\"";;
    esac
done

info "Building boost $BOOST_VERSION..."
cd ${callDir}

set -eu
info "Generating config..."
echo "path-constant ndk : ${ANDROID_NDK_ROOT} ;" > arm64-v8a-config.jam
echo "using clang : 7.0~arm64 : \$(ndk)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android23-clang++ ;" >> arm64-v8a-config.jam

info "Patching..."
patch -p1 < ${ownLocation}/boost_1_69_0_android.patch

info "Bootstrapping..."
#./bootstrap.sh #--with-toolset=clang
./bootstrap.sh #--with-libraries=${BOOST_LIBS_TO_BUILD}

info "Building..."
./b2 -d+2 \
    -j 15 \
    --reconfigure \
    target-os=android \
    toolset=clang-7.0~arm64 \
    link=static \
    variant=release \
    threading=multi \
    cxxflags="-std=c++14 -fPIC" \
    --with-${BOOST_LIBS_TO_BUILD//,/ --with-} \
    --user-config=arm64-v8a-config.jam \
    --prefix=$(pwd)/../boost_${BOOST_VERSION//./_}_android_${ANDROID_ARCH} \
    install

info "Done!"
#read a
