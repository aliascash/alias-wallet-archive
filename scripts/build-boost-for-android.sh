#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
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

ANDROID_NDK_ARCHIVE_LOCATION=${ARCHIVES_ROOT_DIR}/Android
ANDROID_NDK_ROOT=${ANDROID_NDK_ARCHIVE_LOCATION}/android-ndk-${ANDROID_NDK_VERSION}
ANDROID_ARCH=arm64
ANDROID_API=22
BOOST_LIBS_TO_BUILD=chrono

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
    -p <android-API-level>
    -v <version>
        Boost version to build. Default: ${BOOST_VERSION}
    -h  Show this help

    "
}

HOST_SYSTEM='linux'

# Determine system
# Determine amount of cores:
if [[ "$OSTYPE" == "linux-gnu" ]]; then
    CORES_TO_USE=$(grep -c ^processor /proc/cpuinfo)
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    CORES_TO_USE=$(system_profiler SPHardwareDataType | grep "Total Number of Cores" | tr -s " " | cut -d " " -f 6)
    HOST_SYSTEM='darwin'
else
    CORES_TO_USE=1
fi

while getopts a:l:n:p:v:h? option; do
    case ${option} in
        a) ANDROID_ARCH="${OPTARG}";;
        c) CORES_TO_USE="${OPTARG}";;
        l) BOOST_LIBS_TO_BUILD="${OPTARG}";;
        n) ANDROID_NDK_ROOT="${OPTARG}";;
        p) ANDROID_API="${OPTARG}";;
        v) BOOST_VERSION="${OPTARG}";;
        h|?) helpMe && exit 0;;
        *) die 90 "invalid option \"${OPTARG}\"";;
    esac
done

info " -> Boost $BOOST_VERSION..."
cd "${callDir}"

case ${ANDROID_ARCH} in
    arm64)
        jamEntry1="7.0~arm64"
        jamEntry2="aarch64"
        ;;
    *)
        jamEntry1="${ANDROID_ARCH}"
        jamEntry2="${ANDROID_ARCH}"
        ;;
esac

set -eu
info " -> Generating config..."
echo "path-constant ndk : ${ANDROID_NDK_ROOT} ;" > "${ANDROID_ARCH}"-config.jam
if [[ "${ANDROID_ARCH}" = "armv7a" ]] ; then
    echo "using clang : ${jamEntry1} : \$(ndk)/toolchains/llvm/prebuilt/${HOST_SYSTEM}-x86_64/bin/${jamEntry2}-linux-androideabi${ANDROID_API}-clang++ ;" >> "${ANDROID_ARCH}"-config.jam
else
    echo "using clang : ${jamEntry1} : \$(ndk)/toolchains/llvm/prebuilt/${HOST_SYSTEM}-x86_64/bin/${jamEntry2}-linux-android${ANDROID_API}-clang++ ;" >> "${ANDROID_ARCH}"-config.jam
fi

info " -> Bootstrapping..."
#./bootstrap.sh #--with-toolset=clang
./bootstrap.sh #--with-libraries=${BOOST_LIBS_TO_BUILD}

info " -> Building boost with './b2 -d+2 \
    -j ${CORES_TO_USE} \
    --reconfigure \
    target-os=android \
    toolset=clang-${jamEntry1} \
    link=static \
    variant=release \
    threading=multi \
    cxxflags="-std=c++14 -fPIC" \
    --with-${BOOST_LIBS_TO_BUILD//,/ --with-} \
    --user-config=${ANDROID_ARCH}-config.jam \
    --prefix=$(pwd)/../boost_${BOOST_VERSION//./_} \
    install'"
./b2 -d+2 \
    -j "${CORES_TO_USE}" \
    --reconfigure \
    target-os=android \
    toolset=clang-"${jamEntry1}" \
    link=static \
    variant=release \
    threading=multi \
    cxxflags="-std=c++14 -fPIC" \
    --with-${BOOST_LIBS_TO_BUILD//,/ --with-} \
    --user-config="${ANDROID_ARCH}"-config.jam \
    --prefix="$(pwd)"/../boost_"${BOOST_VERSION//./_}" \
    install
info " -> Done!"
#read a
