# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
#
# SPDX-License-Identifier: MIT

### At first perform source build ###
FROM aliascash/alias-wallet-builder-android:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="6"
ARG KEYSTORE_PASS=1234567

# Runtime parameters
ENV BUILD_THREADS=${BUILD_THREADS}
ENV KEYSTORE_PASS=${KEYSTORE_PASS}

COPY . /alias-wallet

WORKDIR /alias-wallet/

RUN ./scripts/cmake-build-android_arm64.sh -g -t ${BUILD_THREADS} \
 && ./scripts/cmake-build-android_armv7.sh -g -t ${BUILD_THREADS} \
 && ./scripts/cmake-build-android_x86-64.sh -g -t ${BUILD_THREADS} \
 && /root/Archives/Android/android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/aarch64-linux-android/bin/strip ./cmake-build-cmdline-android22_arm64/aliaswallet/android-build/libs/arm64-v8a/libAlias_arm64-v8a.so \
 && /root/Archives/Android/android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/arm-linux-androideabi/bin/strip ./cmake-build-cmdline-android22_armv7a/aliaswallet/android-build/libs/armeabi-v7a/libAlias_armeabi-v7a.so \
 && /root/Archives/Android/android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/x86_64-linux-android/bin/strip  ./cmake-build-cmdline-android22_x86_64/aliaswallet/android-build/libs/x86_64/libAlias_x86_64.so

RUN ./scripts/android-prepare-env_CI.sh \
 && ./scripts/android-build.sh

### Now upload binaries to GitHub ###
FROM aliascash/github-uploader:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

ARG GITHUB_CI_TOKEN=1234567
ARG ALIAS_RELEASE=latest
ARG ALIAS_REPOSITORY=alias-wallet
ARG GIT_COMMIT=unknown
ARG REPLACE_EXISTING_ARCHIVE=''
#ENV GITHUB_CI_TOKEN=${GITHUB_CI_TOKEN}
ENV APK=Alias-${ALIAS_RELEASE}-${GIT_COMMIT}-Android.apk
ENV AAB=Alias-${ALIAS_RELEASE}-${GIT_COMMIT}-Android.aab
ENV CHKSUM_FILE_APK=Checksum-Alias-Android-APK.txt
ENV CHKSUM_FILE_AAB=Checksum-Alias-Android-AAB.txt

RUN mkdir -p /filesToUpload

COPY --from=build /alias-wallet/cmake-build-cmdline-android-apk/android-build/build/outputs/apk/release/android-build-release-unsigned.apk /filesToUpload/${APK}
COPY --from=build /alias-wallet/cmake-build-cmdline-android-apk/android-build/build/outputs/bundle/release/android-build-release.aab       /filesToUpload/${AAB}
COPY --from=build /alias-wallet/scripts/createChecksums.sh /tmp/

RUN cd /filesToUpload \
 && github-release upload \
        --user aliascash \
        --security-token "${GITHUB_CI_TOKEN}" \
        --repo "${ALIAS_REPOSITORY}" \
        --tag "${ALIAS_RELEASE}" \
        --name "${APK}" \
        --file "/filesToUpload/${APK}" \
        ${REPLACE_EXISTING_ARCHIVE} \
 && chmod +x /tmp/createChecksums.sh \
 && sh /tmp/createChecksums.sh /filesToUpload/${APK} ${CHKSUM_FILE_APK} \
 && github-release upload \
        --user aliascash \
        --security-token "${GITHUB_CI_TOKEN}" \
        --repo "${ALIAS_REPOSITORY}" \
        --tag "${ALIAS_RELEASE}" \
        --name "${AAB}" \
        --file "/filesToUpload/${AAB}" \
        ${REPLACE_EXISTING_ARCHIVE} \
 && sh /tmp/createChecksums.sh /filesToUpload/${AAB} ${CHKSUM_FILE_AAB} \
 && export GITHUB_CI_TOKEN=---
