#!/bin/bash
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ownLocation}" || exit 1
. ./include/helpers_console.sh
_init
. ./include/handle_buildconfig.sh

info "Alias UI repo:"
if [[ ! -e ${ALIAS_UI_REPO_LOCATION}/alias-wallet-ui ]] ; then
    info " -> Clone of Alias UI repo not found. Cloning it now onto ${ALIAS_UI_REPO_LOCATION}"
    cd ${ALIAS_UI_REPO_LOCATION}/
    git clone https://github.com/aliascash/alias-wallet-ui --branch android
    info " -> Done"
else
    info " -> Using Alias repo clone ${ALIAS_UI_REPO_LOCATION}/alias-wallet-ui"
fi
cd "${ownLocation}"/..
DIR_UI=${ALIAS_UI_REPO_LOCATION}/alias-wallet-ui

info "Setup APK build environment:"
BUILD_DIR_ARM8=cmake-build-cmdline-android${ANDROID_API_ARM64}_arm64
BUILD_DIR_ARM7=cmake-build-cmdline-android${ANDROID_API_ARMV7}_armv7a
BUILD_DIR_x86_64=cmake-build-cmdline-android${ANDROID_API_X86_64}_x86_64

if [ -z "${DEPENDENCIES_BUILD_DIR}" ] ; then
    BUILD_DIR_TOR_ARM8=${BUILD_DIR_ARM8}
    BUILD_DIR_TOR_ARM7=${BUILD_DIR_ARM7}
    BUILD_DIR_TOR_x86_64=${BUILD_DIR_x86_64}
else
    BUILD_DIR_TOR_ARM8=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR_ARM8}
    BUILD_DIR_TOR_ARM7=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR_ARM7}
    BUILD_DIR_TOR_x86_64=${DEPENDENCIES_BUILD_DIR}/${BUILD_DIR_x86_64}
fi

BUILD_DIR_APK=cmake-build-cmdline-android-apk

rm -Rf ${BUILD_DIR_APK}/android-build/assets
mkdir -p ${BUILD_DIR_APK}/android-build
ln -s ${DIR_UI} ${BUILD_DIR_APK}/android-build/assets

mkdir -p ${BUILD_DIR_APK}/android-build/libs/arm64-v8a
cp ${BUILD_DIR_TOR_ARM8}/usr/local/bin/tor ${BUILD_DIR_APK}/android-build/libs/arm64-v8a/libtor.so
cp ${BUILD_DIR_ARM8}/aliaswallet/android-build/libs/arm64-v8a/libAlias_arm64-v8a.so ${BUILD_DIR_APK}/android-build/libs/arm64-v8a/
chmod +x ${BUILD_DIR_APK}/android-build/libs/arm64-v8a/*.so

mkdir -p ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a
cp ${BUILD_DIR_TOR_ARM7}/usr/local/bin/tor ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a/libtor.so
cp ${BUILD_DIR_ARM7}/aliaswallet/android-build/libs/armeabi-v7a/libAlias_armeabi-v7a.so ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a/
chmod +x ${BUILD_DIR_APK}/android-build/libs/armeabi-v7a/*.so

mkdir -p ${BUILD_DIR_APK}/android-build/libs/x86_64
cp ${BUILD_DIR_TOR_x86_64}/usr/local/bin/tor ${BUILD_DIR_APK}/android-build/libs/x86_64/libtor.so
cp ${BUILD_DIR_x86_64}/aliaswallet/android-build/libs/x86_64/libAlias_x86_64.so ${BUILD_DIR_APK}/android-build/libs/x86_64/
chmod +x ${BUILD_DIR_APK}/android-build/libs/x86_64/*.so

sed -e "s#ANDROID_HOME#${ANDROID_HOME}#g" \
    -e "s#ANDROID_NDK_VERSION#${ANDROID_NDK_VERSION}#g" \
    -e "s#QT_INSTALLATION_PATH#${QT_INSTALLATION_PATH}#g" \
    -e "s#QT_VERSION_ANDROID#${QT_VERSION_ANDROID}#g" \
    -e "s#PATH_TO_CLONE#${ownLocation}/..#g" \
    scripts/android_deployment_settings_NEW.json > ${BUILD_DIR_APK}/android_deployment_settings.json

export ANDROID_HOME=${ANDROID_HOME}
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/cmdline-tools/tools/bin:$ANDROID_HOME/platform-tools:/opt/gradle/bin/:$PATH"

sed -i "s#qt5AndroidDir=.*#qt5AndroidDir=${QT_INSTALLATION_PATH}/${QT_VERSION_ANDROID}/android/src/android/java#g" src/android/gradle.properties

# We need at least Gradle 6.5
mkdir -p ${BUILD_DIR_APK}/android-build/gradle/wrapper/
cat >"${BUILD_DIR_APK}/android-build/gradle/wrapper/gradle-wrapper.properties" <<EOF
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https://services.gradle.org/distributions/gradle-6.5-bin.zip
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
EOF
