#!/bin/bash
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ownLocation}"/..
DIR_UI=/Users/teknex/Desktop/spectrecoin/repositories/alias/spectrecoin-ui-android
BUILD_DIR_ARM8=build-spectre_android-Android_Qt_5_15_1_Clang_Multi_Abi_ARMv8-Release
BUILD_DIR_ARM7=build-spectre_android-Android_Qt_5_15_1_Clang_Multi_Abi_ARMv7-Release
BUILD_DIR_x86_64=build-spectre_android-Android_Qt_5_15_1_Clang_Multi_Abi_x86_64-Release

rm -Rf ${BUILD_DIR_ARM8}/android-build/assets
mkdir -p ${BUILD_DIR_ARM8}/android-build
ln -s ${DIR_UI} ${BUILD_DIR_ARM8}/android-build/assets
mkdir -p ${BUILD_DIR_ARM8}/android-build/libs/arm64-v8a
cp cmake-build-cmdline-android26_arm64/usr/local/bin/tor ${BUILD_DIR_ARM8}/android-build/libs/arm64-v8a/libtor.so
chmod +x ${BUILD_DIR_ARM8}/android-build/libs/arm64-v8a/libtor.so

rm -Rf ${BUILD_DIR_ARM7}/android-build/assets
mkdir -p ${BUILD_DIR_ARM7}/android-build
ln -s ${DIR_UI} ${BUILD_DIR_ARM7}/android-build/assets
mkdir -p ${BUILD_DIR_ARM7}/android-build/libs/armeabi-v7a
cp cmake-build-cmdline-android26_armv7a/usr/local/bin/tor ${BUILD_DIR_ARM7}/android-build/libs/armeabi-v7a/libtor.so
chmod +x ${BUILD_DIR_ARM7}/android-build/libs/armeabi-v7a/libtor.so

rm -Rf ${BUILD_DIR_x86_64}/android-build/assets
mkdir -p ${BUILD_DIR_x86_64}/android-build
ln -s ${DIR_UI} ${BUILD_DIR_x86_64}/android-build/assets
mkdir -p ${BUILD_DIR_x86_64}/android-build/libs/x86_64
cp cmake-build-cmdline-android26_x86_64/usr/local/bin/tor ${BUILD_DIR_x86_64}/android-build/libs/x86_64/libtor.so
chmod +x ${BUILD_DIR_x86_64}/android-build/libs/x86_64/libtor.so
