#!/bin/bash -e
aclocal
autoconf
automake --add-missing
pushd tor
./autogen.sh
popd
