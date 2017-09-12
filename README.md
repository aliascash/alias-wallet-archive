SpectreCoin
===========

This is the official source code repository for [Spectrecoin](https://spectreproject.io/) (XSPEC).

Current development activity is on the dev-1.4 branch, for the upcoming 1.4 release.

The latest release is [1.3.3](https://github.com/spectrecoin/spectre/releases/tag/v1.3.3), released on September 12, 2017.

Building on Linux
-----------------

We do not currently provide Linux binary packages. To build the SpectreCoin wallet from source, you will need the following dependencies:

 * OpenSSL 1.0
 * Berkeley DB 4.8
 * Qt 4
 * libevent
 * boost

To build the wallet, run the following commands, adjusting the OpenSSL and Berkeley DB paths to suit your system. If you have many CPU cores, you can increase the `-j` option to speed up the build. On your system, `qmake-qt4` may be called `qmake4` or just `qmake`.

    $ qmake-qt4 OPENSSL_LIB_PATH=/usr/lib/openssl-1.0 OPENSSL_INCLUDE_PATH=/usr/include/openssl-1.0 BDB_INCLUDE_PATH=/usr/include/db4.8 BDB_LIB_SUFFIX=-4.8
    $ make -j2

The resulting binary will be called `spectre` in the current directory.
