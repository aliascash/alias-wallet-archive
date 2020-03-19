#!/bin/bash -e

[[ -d .git ]] && [[ -d external/leveldb ]] && [[ -d external/openssl-cmake ]] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

# We need to get back to #4cb80b7 on leveldb
# as HEAD is not building properly with CMake
cd external/leveldb
git checkout 4cb80b7
cd - >/dev/null

echo "Preparation finished."
echo "To build, just run one of the ./scripts/cmake-build*.sh scripts"
