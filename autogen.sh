#!/bin/bash -e

[ -d .git ] && [ -d tor ] && [ -d leveldb ] && [ -d db4.8 ] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

# Disabled as we are using fully configured repos now!
#autoreconf --no-recursive --install
#
#pushd tor
#./autogen.sh
#popd
