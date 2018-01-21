#!/bin/sh -e

[ -d .git ] && [ -d tor ] && [ -d leveldb ] || \
  { echo "Please run this command from the root of the Spectrecoin repository." && exit 1; }

git submodule init
git submodule sync --recursive
git submodule update --recursive --force

pushd tor && git clean -fdx . && popd
pushd leveldb && git clean -fdx . && popd
pushd db4.8 && git checkout -f . && git clean -fdx . && popd

autoreconf --no-recursive --install

PATCH="patch --no-backup-if-mismatch -f"

pushd tor
$PATCH -p0 < ../tor-or-am.patch
$PATCH -p0 < ../tor-am.patch
./autogen.sh
popd

pushd leveldb
$PATCH -p1 < ../leveldb-memenv.patch
$PATCH -p1 < ../leveldb-harden.patch
$PATCH -p1 < ../leveldb-win32.patch
$PATCH -p1 < ../leveldb-arm64.patch
popd

pushd db4.8
$PATCH -p1 < ../db-atomic.patch
popd
