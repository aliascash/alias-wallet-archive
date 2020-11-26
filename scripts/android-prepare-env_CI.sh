#!/bin/bash
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ ! -e /alias-wallet-ui ]] ; then
    cd /
    git clone https://github.com/aliascash/alias-wallet-ui
fi
cd "${ownLocation}"/..
DIR_UI=/alias-wallet-ui

BUILD_DIR_ARM8=cmake-build-cmdline-android22_arm64
BUILD_DIR_ARM7=cmake-build-cmdline-android22_armv7a
BUILD_DIR_x86_64=cmake-build-cmdline-android22_x86_64

BUILD_DIR_APK=cmake-build-cmdline-android-apk

rm -Rf ${BUILD_DIR_APK}/android-build/assets
mkdir -p ${BUILD_DIR_APK}/android-build
ln -s ${DIR_UI} ${BUILD_DIR_APK}/android-build/assets

mkdir -p ${BUILD_DIR_APK}/android-build/libs/arm64-v8a
cp ${BUILD_DIR_ARM8}/usr/local/bin/tor ${BUILD_DIR_APK}/android-build/libs/arm64-v8a/libtor.so
cp ${BUILD_DIR_ARM8}/aliaswallet/android-build/libs/arm64-v8a/libAlias_arm64-v8a.so ${BUILD_DIR_APK}/android-build/libs/arm64-v8a/
chmod +x ${BUILD_DIR_APK}/android-build/libs/arm64-v8a/*.so

mkdir -p ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a
cp ${BUILD_DIR_ARM7}/usr/local/bin/tor ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a/libtor.so
cp ${BUILD_DIR_ARM7}/aliaswallet/android-build/libs/armeabi-v7a/libAlias_armeabi-v7a.so ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a/
chmod +x ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a/*.so

mkdir -p ${BUILD_DIR_APK}/android-build/libs/x86_64
cp ${BUILD_DIR_x86_64}/usr/local/bin/tor ${BUILD_DIR_APK}/android-build/libs/x86_64/libtor.so
cp ${BUILD_DIR_x86_64}/aliaswallet/android-build/libs/x86_64/libAlias_x86_64.so ${BUILD_DIR_APK}/android-build/libs/x86_64/
chmod +x ${BUILD_DIR_APK}/android-build/libs/x86_64/*.so

cp -f scripts/android_deployment_settings_CI.json ${BUILD_DIR_APK}/android_deployment_settings.json

export ANDROID_HOME=/root/Archives/Android
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/cmdline-tools/tools/bin:$ANDROID_HOME/platform-tools:/opt/gradle/bin/:$PATH"

sed -i '' "s#qt5AndroidDir=.*#qt5AndroidDir=/root/Qt/5.15.2/android/src/android/java#g" src/android/gradle.properties
