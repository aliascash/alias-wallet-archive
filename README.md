# Alias
[![GitHub version](https://badge.fury.io/gh/aliascash%2Fspectre.svg)](https://badge.fury.io/gh/aliascash%2Fspectre) [![HitCount](http://hits.dwyl.io/aliascash/https://github.com/aliascash/aliaswallet.svg)](http://hits.dwyl.io/aliascash/https://github.com/aliascash/aliaswallet)
[![Build Status](https://ci.spectreproject.io/buildStatus/icon?job=Alias/spectre/develop)](https://ci.spectreproject.io/job/Alias/job/spectre/job/develop/)

Alias is a Secure Proof-of-Stake (PoSv3) Network with Anonymous Transaction Capability.

Alias utilizes a range of proven cryptographic techniques to achieve un-linkable,
un-traceable and anonymous transactions on its underlaying blockchain and also protects
the users identity by running all the network nodes as Tor hidden services.

# Licensing

- SPDX-FileCopyrightText: © 2020 Alias Developers
- SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
- SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
- SPDX-FileCopyrightText: © 2014 BlackCoin Developers
- SPDX-FileCopyrightText: © 2013 NovaCoin Developers
- SPDX-FileCopyrightText: © 2011 PPCoin Developers
- SPDX-FileCopyrightText: © 2009 Bitcoin Developers

SPDX-License-Identifier: MIT

# Social
- Visit our website [Alias](https://spectreproject.io/) (XSPEC)
- Please join us on our [Discord](https://discord.gg/ckkrb8m) server
- Read the latest [Newsletter](https://news.spectreproject.io/)
- Visit our thread at [BitcoinTalk](https://bitcointalk.org/index.php?topic=2103301.0)

## Key Privacy Technology

Anonymous token creation: Through the use of dual key stealth technology Alias provides
the ability to generate ‘anonymous tokens’ (SPECTRE) by consuming XSPEC. SPECTRE can then be
sent anonymously through an implementation of ring signatures based on the Cryptonote protocol
to eliminate any transaction history. The wallet offers the opportunity to transfer your
balance between public coins, XSPEC, and ‘anonymous tokens’, SPECTRE. We are currently working
on improving this technology to improve functionality and privacy.

Built in Tor: The Alias software offers a full integration of Tor
(https://www.torproject.org/) so that the Alias client runs exclusive as a Tor
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
testing than released versions. If you want to build a stable version of Alias, please
check out the latest release tag before you start building.

### UI development

The following files where maintained on the separate Git repository
[aliaswallet-ui](https://github.com/aliascash/aliaswallet-ui):
* src/qt/res/assets/*
* src/qt/res/index.html
* spectre.qrc

**Do not modify them here!**

### Dependencies

To build the Alias wallet from source, you will need the following dependencies:

* OpenSSL 1.1
* BerkeleyDB 4.8
* LevelDB
* libevent
* Boost
* Tor & obfs4proxy (since 2.1.0 Tor is run as separate process and thus Tor is only a
 runtime dependency)
* Qt 5 if you want to build the GUI wallet. At runtime Qt Webengine is required but not at built time. Qt is not needed for the console wallet.

Additionally, you'll need the native C/C++ compiler for your platform and the basic
dependencies needed for any kind of development. After the wipe out of Qt Webengine as a compile time dependency, we're working on cross compiling. But at the moment it is not possible.

 * macOS - Xcode with Command Line Tools and clang, QTs QMAKE
 * Windows - [vcpkg](https://github.com/Microsoft/vcpkg) and MSVC, QTs QMAKE
 * Linux - GCC

### Windows

The Windows wallet is build with QTs **QMAKE**. See the [instructions for Windows on our Wiki](https://github.com/aliascash/documentation/wiki/Build-Windows).

### macOS

To check all dependencies and install missing ones on **macOS** (this uses the
[Homebrew](https://brew.sh/) package manager; if you use something else then adjust
the commands accordingly):

```
brew install autoconf automake libtool pkg-config openssl@1.1 libevent boost gcc wget
```

The macOS wallet itself is build with QTs **QMAKE**. See
https://github.com/aliascash/aliaswallet/blob/develop/src/osx.pri for instructions.

### Linux

Check out the documents for different Linux distributions on our [Wiki](https://github.com/aliascash/documentation/wiki).
