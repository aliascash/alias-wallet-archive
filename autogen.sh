#!/bin/bash -e
pushd tor && git checkout -f . && popd
pushd leveldb && git checkout -f . && popd
git submodule update --init

autoreconf --no-recursive --install

pushd tor
patch --no-backup-if-mismatch -f -p0 < ../tor-or-am.patch
patch --no-backup-if-mismatch -f -p0 < ../tor-am.patch
./autogen.sh
popd

pushd leveldb
patch --no-backup-if-mismatch -f -p1 < ../leveldb-win32.patch
popd
