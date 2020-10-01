#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2019-04-22 HLXEasy
#
# Helper script to download UI asset archive from our CI
# and update existing assets with content of this archive
#
# ===========================================================================

# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd ${ownLocation}
. ./include/helpers_console.sh

# Go to Aliaswallet repository root directory
cd ..

_init

if [[ -z "$1" ]] ; then
    downloadURL=https://ci.alias.cash/job/Alias/job/alias-wallet-ui/job/Aliaswallet-UI/lastSuccessfulBuild/artifact/alias-wallet-ui-assets.tgz
else
    downloadURL=$1
fi

info "Using download URL ${downloadURL}"
cd src/qt/res
wget ${downloadURL}

info "Updating content"
tar xzf ${downloadURL##*/}
mv spectre.qrc ../../../

info "Cleanup"
rm -f ${downloadURL##*/}
