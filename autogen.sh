#!/bin/sh -e
aclocal
autoconf
automake
pushd tor
./autogen.sh
popd
