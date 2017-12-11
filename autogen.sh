#!/bin/bash -e
git submodule update --init

aclocal
autoconf
automake --add-missing

pushd tor
git checkout -f .
patch --no-backup-if-mismatch -f -p0 < ../tor-or-am.patch
patch --no-backup-if-mismatch -f -p0 < ../tor-am.patch
./autogen.sh
popd
