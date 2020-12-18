#!/bin/bash
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$ownLocation"/../cmake-build-cmdline-android-apk/ || exit 1

if [[ -z ${KEYSTORE_PASS} ]] ; then
    read -r -s -p "Please enter sign keystore password: " KEYSTORE_PASS
    echo
fi

/root/Qt/5.15.2/android/bin/androiddeployqt \
    --input $(pwd)/android_deployment_settings.json \
    --output $(pwd)/android-build \
    --android-platform android-30 \
    --gradle \
    --aab \
    --jarsigner \
    --sign /etc/ssl/certs/alias-sign-keystore.jks upload \
    --storepass "${KEYSTORE_PASS}" \
      | tee ../Android-Build-$(date +%Y-%m-%d_%H%M%S).log ; \
      exit "${PIPESTATUS[0]}"
