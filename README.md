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
 * libevent
 * boost
 * Qt 4 if you want to build the GUI wallet. Qt is not needed for the console wallet.

To build the wallet, run the following commands:

    $ git submodule update --init  # to check out the Tor and LevelDB dependencies
    $ autoconf
    $ ./configure --enable-gui  # leave out --enable-gui to build only the console wallet
    $ make -j2

The resulting binaries will be in the `src` directory and called `spectre` for the GUI wallet and `spectrecoind` for the console wallet.

If your distro ships OpenSSL 1.1 as the default version of OpenSSL, you'll need to install OpenSSL 1.0 and pass the correct paths to `configure`. An example for Arch Linux is:

    $ ./configure --enable-gui LDFLAGS="-L/usr/lib/openssl-1.0" CFLAGS="-I/usr/include/openssl-1.0 -O2" CPPFLAGS="-I/usr/include/openssl-1.0 -O2"
