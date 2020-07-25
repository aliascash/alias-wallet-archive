#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2018-11-28 HLXEasy
#
# This script can be used to build Spectrecoin on Mac
#
# ===========================================================================

# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd ${ownLocation}
. ./include/helpers_console.sh

# Go to Spectrecoin repository root directory
cd ..

if [[ -z "${QT_PATH}" ]] ; then
    QT_PATH=~/Qt/5.9.6/clang_64
    warning "QT_PATH not set, using '${QT_PATH}'"
else
    info "QT_PATH: ${QT_PATH}"
fi
if [[ -z "${OPENSSL_PATH}" ]] ; then
    OPENSSL_PATH=/usr/local/Cellar/openssl@1.1/1.1.1d
    warning "OPENSSL_PATH not set, using '${OPENSSL_PATH}'"
else
    info "OPENSSL_PATH: ${OPENSSL_PATH}"
fi

if [[ -z "${BOOST_PATH}" ]] ; then
    BOOST_PATH=/usr/local/Cellar/boost/1.68.0_1
    warning "BOOST_PATH not set, using '${BOOST_PATH}'"
else
    info "BOOST_PATH: ${BOOST_PATH}"
fi

info "Calling autogen.sh"
./autogen.sh

info "Configure and make db4.8:"
cd db4.8/build_unix/
./configure --enable-cxx --disable-shared --disable-replication --with-pic && make

info "Configure and make leveldb:"
cd ../../leveldb/
./build_detect_platform build_config.mk ./ && make
cd ../

info "Starting qmake:"
${QT_PATH}/bin/qmake src/src.pro -spec macx-clang CONFIG+=x86_64
make -j2
