SpectreCoin
===========

This is the official source code repository for [Spectrecoin](https://spectreproject.io/) (XSPEC).

The latest release is [1.3.3](https://github.com/spectrecoin/spectre/releases/tag/v1.3.3), released on September 12, 2017.

Building on Linux
-----------------

We do not currently provide Linux binary packages. To build the SpectreCoin wallet from source, you will need the following dependencies:

 * OpenSSL 1.0
 * libevent
 * boost
 * Qt 4 if you want to build the GUI wallet. Qt is not needed for the console wallet.

To build the wallet, run the following commands:

    $ ./autogen.sh
    $ ./configure --enable-gui  # leave out --enable-gui to build only the console wallet
    $ make -j2

If your distribution provides both OpenSSL 1.0 and OpenSSL 1.1, you'll need to use the `PKG_CONFIG_PATH` environment variable to point `configure` to the directory that contains the `openssl.pc` file for OpenSSL 1.0. For example:

    $ PKG_CONFIG_PATH=/usr/lib/openssl-1.0/pkgconfig ./configure --enable-gui

Please note that building with `clang` is not currently supported, due to a limitation in Berkeley DB. If `clang` is the default compiler on your system please use the `CC` and `CXX` environment variables when calling `configure` to select `gcc` instead like this:

    $ CC=gcc CXX=g++ ./configure --enable-gui

The resulting binaries will be in the `src` directory and called `spectre` for the GUI wallet and `spectrecoind` for the console wallet.
