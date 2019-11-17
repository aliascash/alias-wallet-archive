#!/bin/bash

BOOST_VERSION=1.68.0

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
    -v <version>
        Boost version to build. Default: ${BOOST_VERSION}
    -h  Show this help

    "
}

_init

info "Building boost $BOOST_VERSION..."

while getopts v:h? option; do
    case ${option} in
        v) BOOST_VERSION="${OPTARG}";;
        h|?) helpMe && exit 0;;
        *) die 90 "invalid option \"${OPTARG}\"";;
    esac
done

set -eu

toolchain=$PWD/toolchain
if [[ ! -d "$toolchain" ]]; then
    info "Building toolchain..."
    ${ANDROID_NDK_ROOT}/build/tools/make-standalone-toolchain.sh \
        --arch=arm --platform=android-21 \
        --install-dir="$toolchain" \
        --toolchain=arm-linux-androideabi-clang \
        --use-llvm --stl=libc++
else
    info "Toolchain already built"
fi

cd ${callDir}

info "Generating config..."
user_config=tools/build/src/user-config.jam
rm -f ${user_config}
cat <<EOF > ${user_config}
import os ;

using clang : android
:
"${toolchain}/bin/clang++"
:
<archiver>${toolchain}/bin/arm-linux-androideabi-ar
<ranlib>${toolchain}/bin/arm-linux-androideabi-ranlib
;
EOF

info "Bootstrapping..."
./bootstrap.sh #--with-toolset=clang

info "Building..."
./b2 -j32 \
    toolset=clang-android \
    architecture=arm \
    variant=release \
    --layout=versioned \
    target-os=android \
    threading=multi \
    threadapi=pthread \
    link=static \
    runtime-link=static \

info "Running ranlib on libraries..."
libs=$(find "bin.v2/libs/" -name '*.a')
for lib in ${libs}; do
  "$toolchain/bin/arm-linux-androideabi-ranlib" "$lib"
done

info "Done!"
