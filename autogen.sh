#!/bin/bash -e

[[ -d .git ]] && [[ -d leveldb ]] && [[ -d extern/openssl-cmake ]] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

echo "Preparation finished."
echo "To build, just run ./scripts/cmake-build.sh or ./scripts/cmake-build-android.sh"
