# Spectrecoin
[![GitHub version](https://badge.fury.io/gh/spectrecoin%2Fspectre.svg)](https://badge.fury.io/gh/spectrecoin%2Fspectre) [![HitCount](http://hits.dwyl.io/spectrecoin/https://github.com/spectrecoin/spectre.svg)](http://hits.dwyl.io/spectrecoin/https://github.com/spectrecoin/spectre)
[![Build Status](https://ci.spectreproject.io/buildStatus/icon?job=Spectrecoin/spectre/develop)](https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/develop/)

Spectrecoin is a Secure Proof-of-Stake (PoSv3) Network with Anonymous Transaction Capability.

Spectrecoin utilizes a range of proven cryptographic techniques to achieve un-linkable,
un-traceable and anonymous transactions on its underlaying blockchain and also protects
the users identity by running all the network nodes as Tor hidden services.

# Social
- Visit our website [Spectrecoin](https://spectreproject.io/) (XSPEC)
- Please join us on our [Discord](https://discord.gg/ckkrb8m) server
- Read the latest [Newsletter](https://news.spectreproject.io/)
- Visit our thread at [BitcoinTalk](https://bitcointalk.org/index.php?topic=2103301.0)

## Key Privacy Technology

Anonymous token creation: Through the use of dual key stealth technology Spectrecoin provides
the ability to generate ‘anonymous tokens’ (SPECTRE) by consuming XSPEC. SPECTRE can then be
sent anonymously through an implementation of ring signatures based on the Cryptonote protocol
to eliminate any transaction history. The wallet offers the opportunity to transfer your
balance between public coins, XSPEC, and ‘anonymous tokens’, SPECTRE. We are currently working
on improving this technology to improve functionality and privacy.

Built in Tor: The Spectrecoin software offers a full integration of Tor
(https://www.torproject.org/) so that the Spectrecoin client runs exclusive as a Tor
hidden service using a .onion address to connect to other clients in the network. Your
real IP address is therefore protected at all times.

## Basic Coin Specs V3/V4
<table>
<tr><td>Algo</td><td>PoSv3/PoAS</td></tr>
<tr><td>Block Time</td><td>96 Seconds</td></tr>
<tr><td>Difficulty Retargeting</td><td>Every Block (Moving average of last 24 hours)</td></tr>
<tr><td>Initial Coin Supply</td><td>20,000,000 XSPEC</td></tr>
<tr><td>Funding Coin Supply</td><td>3,000,000 XSPEC</td></tr>
<tr><td>Max Coin Supply (PoS Phase)</td><td>fix 3 SPECTRE or 2 XSPEC reward per block</td></tr>
<tr><td>Min Stake Maturity</td><td>450 blocks (~12 hours)</td></tr>
<tr><td>Min SPECTRE Confirmations</td><td>10 blocks</td></tr>
<tr><td>Base Fee</td><td>0.0001 SPECTRE/XSPEC</td></tr>
<tr><td>Max Anon Output</td><td>1000</td></tr>
<tr><td>Ring Size</td><td>fix 10</td></tr>
</table>

## Building from source

**NOTE** that these instructions are relevant for building from develop, which is the latest
code in development. It is generally stable but can contain features that have had less
testing than released versions. If you want to build a stable version of Spectrecoin, please
check out the latest release tag before you start building.

### UI development

The following files where maintained on the separate Git repository
[spectrecoin-ui](https://github.com/spectrecoin/spectrecoin-ui):
* src/qt/res/assets/*
* src/qt/res/index.html
* spectre.qrc

**Do not modify them here!**

### Dependencies

To build the Spectrecoin wallet from source, you will need the following dependencies:

 * OpenSSL 1.1
 * libevent
 * libseccomp
 * libcap
 * boost
 * tor & obfs4proxy (since 2.1.0 tor is run as separate process and thus tor is only a
 runtime dependency)
 * Qt 5 with Qt Webengine if you want to build the GUI wallet. Qt is not needed for the
 console wallet.

Additionally, you'll need the native C/C++ compiler for your platform and the basic
dependencies needed for any kind of development. Because of Qt Webengine cross compiling
is currently not possible.

 * macOS - Xcode with Command Line Tools and clang, QTs QMAKE
 * Windows - [vcpkg](https://github.com/Microsoft/vcpkg) and MSVC, QTs QMAKE
 * Linux - GCC

### Windows

The Windows wallet is build with QTs **QMAKE**. Instructions are found in separate doc
at https://github.com/spectrecoin/spectre/blob/master/doc/Windows-build-instructions-README.md

### macOS

To check all dependencies and install missing ones on **macOS** (this uses the
[Homebrew](https://brew.sh/) package manager; if you use something else then adjust
the commands accordingly):

```
brew install autoconf automake libtool pkg-config openssl@1.1 libevent boost gcc wget
```

The macOS wallet itself is build with QTs **QMAKE**. See
https://github.com/spectrecoin/spectre/blob/develop/src/osx.pri for instructions.

### Linux

#### Preconditions, Dependencies

Check the dockerfile of your corresponding platform to get the list of packages which
must be installed on your system. You can find them here:

To _build_ Spectrecoin (build time dependencies):
 * Debian Stretch - https://github.com/spectrecoin/spectre-builder/blob/develop/Debian/Dockerfile_Stretch
 * Debian Buster - https://github.com/spectrecoin/spectre-builder/blob/develop/Debian/Dockerfile_Buster
 * Fedora - https://github.com/spectrecoin/spectre-builder/blob/develop/Fedora/Dockerfile
 * RasperryPi Stretch - https://github.com/spectrecoin/spectre-builder/blob/develop/RaspberryPi/Dockerfile_Stretch
 * RasperryPi Buster - https://github.com/spectrecoin/spectre-builder/blob/develop/RaspberryPi/Dockerfile_Buster
 * Ubuntu 18.04 - https://github.com/spectrecoin/spectre-builder/blob/develop/Ubuntu/Dockerfile_18_04
 * Ubuntu 19.04 - https://github.com/spectrecoin/spectre-builder/blob/develop/Ubuntu/Dockerfile_19_04

To _run_ Spectrecoin (run time dependencies):
 * Debian Stretch - https://github.com/spectrecoin/spectre-base/blob/develop/Debian/Dockerfile_Stretch
 * Debian Buster - https://github.com/spectrecoin/spectre-base/blob/develop/Debian/Dockerfile_Buster
 * Fedora - https://github.com/spectrecoin/spectre-base/blob/develop/Fedora/Dockerfile
 * RasperryPi Stretch - https://github.com/spectrecoin/spectre-base/blob/develop/RaspberryPi/Dockerfile_Stretch
 * RasperryPi Buster - https://github.com/spectrecoin/spectre-base/blob/develop/RaspberryPi/Dockerfile_Buster
 * Ubuntu 18.04 - https://github.com/spectrecoin/spectre-base/blob/develop/Ubuntu/Dockerfile_18_04
 * Ubuntu 19.04 - https://github.com/spectrecoin/spectre-base/blob/develop/Ubuntu/Dockerfile_19_04

#### Building

To fetch the source code and build the wallet run the following commands:

```
git clone --recursive https://github.com/spectrecoin/spectre
cd spectre
./autogen.sh
./configure --enable-gui
make -j2  # Use a higher number if you have many cores and memory, leave
          # '-j2' out if you are on a very low-powered system like Raspberry Pi
```

The resulting binaries will be in the `src` directory and called `spectre` for the GUI
wallet and `spectrecoind` for the console wallet.

Distribution specfic instructions are found in the corresponding dockerfile. See
https://github.com/spectrecoin/spectre/tree/develop/Docker

## Docker
### Using Docker to _run_ spectrecoind

We provide a ready to run Docker image, which is mainly build out of another Git
repository. For details see [docker-spectrecoind](https://github.com/spectrecoin/docker-spectrecoind)

### Using Docker to _build_ spectrecoind

As long as cross compiling is not possible (due to [QtWebEngine](https://wiki.qt.io/QtWebEngine))
we build the binaries for different Linux distributions using different Docker images.

Actually it is not possible to use this mechanism locally out of the box, as the binaries
stay inside the created image.

More documentation regarding how to use will be on our [Wiki](https://github.com/spectrecoin/documentation/wiki).
