#!/bin/bash
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$ownLocation"/../cmake-build-cmdline-android-apk/ || exit 1

if [[ -z ${keystorePass} ]] ; then
    read -r -s -p "Please enter sign keystore password: " keystorePass
    echo
fi

/root/Qt/5.15.2/android/bin/androiddeployqt \
    --input $(pwd)/android_deployment_settings.json \
    --output $(pwd)/android-build \
    --android-platform android-29 \
    --gradle \
    --aab \
	--jarsigner \
	--sign /etc/ssl/certs/alias-sign-keystore.jks upload \
	--storepass "${keystorePass}" \
    | tee ../Android-Build-$(date +%Y-%m-%d_%H%M%S).log
