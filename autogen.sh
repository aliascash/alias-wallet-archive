#!/bin/bash -e

[ -d .git ] && [ -d leveldb ] && [ -d db4.8 ] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

autoreconf --no-recursive --install

#pushd tor
#./autogen.sh
#popd
