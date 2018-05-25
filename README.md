SpectreCoin (XSPEC)
===================
[![GitHub version](https://badge.fury.io/gh/spectrecoin%2Fspectre.svg)](https://badge.fury.io/gh/spectrecoin%2Fspectre)

Spectrecoin (XSPEC) is a Secure Proof-of-Stake (PoSv3) Network with Anonymous Transaction Capability.

XSPEC utilizes a range of proven cryptographic techniques to achieve un-linkable and anonymous transactions on its underlaying blockchain and also protects the users identity.

Social
=======
- Visit our website [Spectrecoin](https://spectreproject.io/) (XSPEC).
- Please join us on our [Discord](https://discord.gg/ckkrb8m) server!
- Visit our thread at [BitcoinTalk](https://bitcointalk.org/index.php?topic=2103301.0)

### Key Privacy Technology
<table>
<tr><td>Zerocoin Protocol</td><td>Provides the ability to generate ‘anonymous tokens’ (SPECTRE) by burning coins and hence eliminate any transaction history. SPECTRE can then be sent anonymously.</td></tr>
<tr><td>Cryptonote Protocol</td><td>Provides dual-key stealth addresses and ring signatures to further anonymise the transactions conducted with SPECTRE.</td></tr>
<tr><td>Built in Tor</td><td>Full integration of Tor ([The Tor Project](https://www.torproject.org/)) so that the whole network runs as hidden services only. There are no exit nodes and your identity is protected at all times.</td></tr>
</table>



### Basic Coin Specs
<table>
<tr><td>Algo</td><td>PoSv3</td></tr>
<tr><td>Block Time</td><td>60 Seconds</td></tr>
<tr><td>Difficulty Retargeting</td><td>Every Block</td></tr>
<tr><td>Initial Coin Supply</td><td>20,000,000 XSPEC</td></tr>
<tr><td>Max Coin Supply (PoS Phase)</td><td>5% annual inflation</td></tr>
</table>

Building from source
====================

**NOTE** that these instructions are relevant for building from master, which is the latest code in development. It is generally stable but can contain features that have had less testing than released versions. If you want to build a stable version of Spectre, please check out the latest release tag (v1.3.5) before you start building.

Dependencies
------------

We do not currently provide Linux binary packages. To build the SpectreCoin wallet from source, you will need the following dependencies:

 * OpenSSL 1.1
 * libevent
 * libseccomp
 * libcap
 * boost
 * Qt 5 if you want to build the GUI wallet. Qt is not needed for the console wallet.

Additionally, you'll need a C/C++ compiler and the basic dependencies needed for any kind of development. On most Linux distributions there is a metapackage that installs these. On macOS this means you will need Xcode and the Command Line Tools.

To check all dependencies and install missing ones on **Debian/Ubuntu/Mint/etc**:

    apt install build-essential libssl-dev libevent-dev libseccomp-dev libcap-dev pkg-config autoconf libtool

To build a GUI wallet for **Debian/Ubuntu/Mint/etc**:

    Work in progress

To check all dependencies and install missing ones on **Arch Linux**:

    pacman -S --needed base-devel openssl libevent libseccomp libcap boost
    pacman -S --needed qt5  # only if building the GUI wallet

To check all dependencies and install missing ones on **macOS** (this uses the [Homebrew](https://brew.sh/) package manager; if you use something else then adjust the commands accordingly):

    brew install autoconf automake libtool pkg-config openssl@1.1 libevent boost gcc wget
    # the following commands are only needed if building the GUI wallet
    brew tap KDE-mac/homebrew-kde
    brew install qt
    
Open SSL on Ubuntu and OSX (If you do not have OpenSSL 1.1)
------------

For Ubuntu 16.04 LTS through to 17.10 Open SSL 1.1 isn't available in the repositories and has Version 1.0 installed by default. To install the [latest stable version](https://www.openssl.org/source/) you can build this dependency from source:

    wget https://www.openssl.org/source/openssl-1.1.0h.tar.gz
    tar xzvf openssl-1.1.0h.tar.gz
    cd openssl-1.1.0h
    ./config -Wl,--enable-new-dtags,-rpath,'$(LIBRPATH)'
    make
    sudo make install

Building
--------

If you are trying to install the GUI wallet. Install Qt from https://www.qt.io/download-qt-installer and write down the installation path to use in the below ./configure command. Make sure to pick QtWebEngine as well when installing Qt

To fetch the source code and build the wallet run the following commands:

    git clone --recursive https://github.com/spectrecoin/spectre
    cd spectre
    ./install_boost_1_67  # only necessary on Debian/Ubuntu/Mint/etc. If you have installed Boost 1_67 before, you may ignore this command for now
    ./autogen.sh
    ./configure --enable-gui --with-qt5=/path/to/qt/version/compiler  # leave out --enable-gui to build only the console wallet
    make -j2  # use a higher number if you have many cores and memory, leave -j2 out if you are on a very low-powered system like Raspberry Pi

The resulting binaries will be in the `src` directory and called `spectre` for the GUI wallet and `spectrecoind` for the console wallet.

Cross-compiling for Windows is supported using MingW64, by passing the appropriate `--host` parameter to `./configure`.
