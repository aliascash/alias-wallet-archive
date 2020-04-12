#!/bin/bash -e

[[ -d .git ]] && [[ -d external/openssl-cmake ]] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

echo "Preparation finished."
echo "To build, just run one of the ./scripts/cmake-build*.sh scripts"
