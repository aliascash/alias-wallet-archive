#!/bin/bash

ANDROID_STANDALONE_TOOLCHAIN_ROOT=/home/spectre/Android/standalone_toolchains
BOOST_VERSION=1.68.0
BOOST_LIBS_TO_BUILD=chrono
TOOLCHAIN_ARCH=arm64

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
    -t <path>
        Path to directory which should contain the different toolchains.
    -v <version>
        Boost version to build. Default: ${BOOST_VERSION}
    -h  Show this help

    "
}

_init

while getopts a:l:t:v:h? option; do
    case ${option} in
        a) TOOLCHAIN_ARCH="${OPTARG}";;
        l) BOOST_LIBS_TO_BUILD="${OPTARG}";;
        t) ANDROID_STANDALONE_TOOLCHAIN_ROOT="${OPTARG}";;
        v) BOOST_VERSION="${OPTARG}";;
        h|?) helpMe && exit 0;;
        *) die 90 "invalid option \"${OPTARG}\"";;
    esac
done

info "Building boost $BOOST_VERSION..."

set -eu

if [[ ! -d "${ANDROID_STANDALONE_TOOLCHAIN_ROOT}/${TOOLCHAIN_ARCH}" ]]; then
    info "Building toolchain..."
    ${ANDROID_NDK_ROOT}/build/tools/make-standalone-toolchain.sh \
        --arch=${TOOLCHAIN_ARCH} --platform=android-21 \
        --install-dir="${ANDROID_STANDALONE_TOOLCHAIN_ROOT}/${TOOLCHAIN_ARCH}" \
        --toolchain=${TOOLCHAIN_ARCH//64/}-linux-androideabi-clang \
        --use-llvm --stl=libc++
else
    info "Toolchain already built"
fi

cd ${callDir}

info "Generating config..."
user_config=~/user-config.jam
rm -f ${user_config}
cat <<EOF > ${user_config}
# user-config.jam file
# Defines which standalone toolchains to use for building.

standaloneToolchains = ${ANDROID_STANDALONE_TOOLCHAIN_ROOT} ;

# Uncomment to build using preferred toolchain.

# ARM, ARMV7-A, ARM64
# using clang : arm : \$(standaloneToolchains)/arm/bin/clang++ ;
# using clang : arm : \$(standaloneToolchains)/arm/bin/clang++ <compileflags>-march=armv7-a;
# using clang : arm64 : \$(standaloneToolchains)/arm64/bin/clang++ ;

# MIPS, MIPS64
# using clang : mips : \$(standaloneToolchains)/mips/bin/clang++ ;
# using clang : mips64 : \$(standaloneToolchains)/mips64/bin/clang++ ;

# x86, x86_64
# using clang : x86 : \$(standaloneToolchains)/x86/bin/clang++ ;
# using clang : x86_64 : \$(standaloneToolchains)/x86_64/bin/clang++ ;

using clang : ${TOOLCHAIN_ARCH} : \$(standaloneToolchains)/${TOOLCHAIN_ARCH}/bin/clang++ ;

#import os ;
#
#using clang : android
#:
#"\$(standaloneToolchains)/bin/clang++"
#:
#<archiver>\$(standaloneToolchains)/bin/arm-linux-androideabi-ar
#<ranlib>\$(standaloneToolchains)/bin/arm-linux-androideabi-ranlib
#;
EOF

info "Bootstrapping..."
#./bootstrap.sh #--with-toolset=clang
./bootstrap.sh --with-libraries=${BOOST_LIBS_TO_BUILD}

info "Building..."
#./b2 -j32 \
#    toolset=clang-android \
#    architecture=${TOOLCHAIN_ARCH//64/} \
#    variant=release \
#    --layout=versioned \
#    target-os=android \
#    threading=multi \
#    threadapi=pthread \
#    link=static \
#    runtime-link=static \

./b2 -d+2 \
    -j 15 \
    --reconfigure \
    target-os=android \
    toolset=clang-arm64 \
    include=${ANDROID_STANDALONE_TOOLCHAIN_ROOT}/${TOOLCHAIN_ARCH}/include/c++/4.9.x \
    link=static,shared \
    variant=release \
    threading=multi \
    --with-${BOOST_LIBS_TO_BUILD//,/ --with-} \
    --prefix=$(pwd)/../boost_${BOOST_VERSION//./_}_android_${TOOLCHAIN_ARCH} \
    install

info "Running ranlib on libraries..."
libs=$(find "bin.v2/libs/" -name '*.a')
for lib in ${libs}; do
  "${ANDROID_STANDALONE_TOOLCHAIN_ROOT}/${TOOLCHAIN_ARCH}/bin/arm-linux-androideabi-ranlib" "$lib"
done

info "Done!"
