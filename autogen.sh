#!/bin/sh -e
aclocal
autoconf
automake --add-missing
pushd tor
./autogen.sh
popd
