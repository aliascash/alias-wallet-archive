#!/bin/bash -e
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# ===========================================================================

[ -d .git ] && [ -d leveldb ] && [ -d db4.8 ] || \
  { echo "Please run this command from the root of the Aliaswallet repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

autoreconf --no-recursive --install

#pushd tor
#./autogen.sh
#popd

# Create build.h
./scripts/genbuild.sh src/build.h