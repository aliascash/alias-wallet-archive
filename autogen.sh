#!/bin/bash -e

[ -d .git ] && [ -d leveldb ] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force --remote

#branchToUse=cmake-migration
#for submodule in db4.8 leveldb tor ; do
#    cd ${submodule}
#    git checkout ${branchToUse}
#    cd -
#done

# Disabled as we are using fully configured repos now!
#autoreconf --no-recursive --install
#
#pushd tor
#./autogen.sh
#popd

# Create build.h
#./share/genbuild.sh src/build.h