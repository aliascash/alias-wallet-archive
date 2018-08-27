# Spectre
[![GitHub version](https://badge.fury.io/gh/spectrecoin%2Fspectre.svg)](https://badge.fury.io/gh/spectrecoin%2Fspectre) [![HitCount](http://hits.dwyl.io/spectrecoin/https://github.com/spectrecoin/spectre.svg)](http://hits.dwyl.io/spectrecoin/https://github.com/spectrecoin/spectre)

Spectre is a Secure Proof-of-Stake (PoSv3) Network with Anonymous Transaction Capability.

Spectre utilizes a range of proven cryptographic techniques to achieve un-linkable, un-traceable and anonymous transactions on its underlaying blockchain and also protects the users identity by running all the network nodes as Tor hidden services.

# Social
- Visit our website [Spectrecoin](https://spectreproject.io/) (XSPEC).
- Please join us on our [Discord](https://discord.gg/ckkrb8m) server!
- Visit our thread at [BitcoinTalk](https://bitcointalk.org/index.php?topic=2103301.0)

## Key Privacy Technology

Anonymous token creation: Through the use of dual key stealth technology Spectre provides the ability to generate ‘anonymous tokens’ (SPECTRE) by consuming XSPEC. SPECTRE can then be sent anonymously through an implementation of ring signatures based on the Cryptonote protocol to eliminate any transaction history. The wallet offers the opportunity to transfer your balance between public coins, XSPEC, and ‘anonymous tokens’, SPECTRE. We are currently working on improving this technology to improve functionality and privacy.

Built in Tor: The Spectre software offers a full integration of Tor (https://www.torproject.org/) so that the Spectre client runs as a Tor hidden service using a .onion address to connect to other clients in the network. Your real IP address is therefore protected at all times.

## Basic Coin Specs
<table>
<tr><td>Algo</td><td>PoSv3</td></tr>
<tr><td>Block Time</td><td>60 Seconds</td></tr>
<tr><td>Difficulty Retargeting</td><td>Every Block</td></tr>
<tr><td>Initial Coin Supply</td><td>20,000,000 XSPEC</td></tr>
<tr><td>Max Coin Supply (PoS Phase)</td><td>5% annual inflation</td></tr>
</table>

## Building from source

**NOTE** that these instructions are relevant for building from master, which is the latest code in development. It is generally stable but can contain features that have had less testing than released versions. If you want to build a stable version of Spectre, please check out the latest release tag (v1.3.5) before you start building.

### Dependencies

We do not currently provide Linux binary packages. To build the SpectreCoin wallet from source, you will need the following dependencies:

 * OpenSSL 1.1
 * libevent
 * libseccomp
 * libcap
 * boost
 * Qt 5 if you want to build the GUI wallet. Qt is not needed for the console wallet.

Additionally, you'll need a C/C++ compiler and the basic dependencies needed for any kind of development. On most Linux distributions there is a metapackage that installs these. On macOS this means you will need Xcode and the Command Line Tools.

**Windows**

To compile for windows we have a new docs for that. Head to https://github.com/spectrecoin/spectre/blob/master/doc/Windows-build-instructions-README.md

To build a GUI wallet for **All platforms (Ignore if you just want to build a CLI/Non GUI version of the wallet)**:

If you are trying to compile the GUI wallet. Install Qt from https://www.qt.io/download-qt-installer and write down the installation path to use in the below ./configure command. Make sure to pick QtWebEngine as well when installing Qt

To check all dependencies and install missing ones on **Debian/Ubuntu/Mint/etc**:

    apt install build-essential libssl-dev libevent-dev libseccomp-dev libcap-dev pkg-config autoconf libtool

To check all dependencies and install missing ones on **Arch Linux**:

    pacman -S --needed base-devel openssl libevent libseccomp libcap boost
    pacman -S --needed qt5  # only if building the GUI wallet

To check all dependencies and install missing ones on **macOS** (this uses the [Homebrew](https://brew.sh/) package manager; if you use something else then adjust the commands accordingly):

    brew install autoconf automake libtool pkg-config openssl@1.1 libevent boost gcc wget
    # the following commands are only needed if building the GUI wallet
    brew tap KDE-mac/homebrew-kde
    brew install qt
    
### Open SSL on Ubuntu and OSX (If you do not have OpenSSL 1.1)

For Ubuntu 16.04 LTS through to 17.10 Open SSL 1.1 isn't available in the repositories and has Version 1.0 installed by default. To install the [latest stable version](https://www.openssl.org/source/) you can build this dependency from source:

    wget https://www.openssl.org/source/openssl-1.1.0h.tar.gz
    tar xzvf openssl-1.1.0h.tar.gz
    cd openssl-1.1.0h
    ./config -Wl,--enable-new-dtags,-rpath,'$(LIBRPATH)'
    make
    sudo make install

### Building

To fetch the source code and build the wallet run the following commands:

    git clone --recursive https://github.com/spectrecoin/spectre
    cd spectre
    ./install_boost_1_67  # only necessary on Debian/Ubuntu/Mint/etc. If you have installed Boost 1_67 before, you may ignore this command for now
    ./autogen.sh
    ./configure --enable-gui --with-qt5=/path/to/qt/version/compiler  # leave out --enable-gui to build only the console wallet
    make -j2  # use a higher number if you have many cores and memory, leave -j2 out if you are on a very low-powered system like Raspberry Pi

The resulting binaries will be in the `src` directory and called `spectre` for the GUI wallet and `spectrecoind` for the console wallet.

Cross-compiling for Windows is supported using MingW64, by passing the appropriate `--host` parameter to `./configure`.

### Using Docker

This repository contains a Dockerfile to create a ready to use image with spectrecoind. The image is based on our 
spectre base image [spectreproject/spectre-base](https://github.com/spectrecoin/spectre-base), which is based on 
Debian on his side. A dedicated user _spectre_ with UID _1000_ and GID _1000_ is used to run [spectrecoind](https://github.com/spectrecoin/spectre).


#### Start new container
You should prepare a directory on the host machine, which contains all 
spectrecoind data. Per default this is the directory _~/.spectrecoin_ and you
can use this directory also with the Docker container by mounting it into the
container.

Explanation of start command:

```
docker run \
    --name <name> \                 # Name of the container to create
    --rm \                          # Container will automatically be removed after stop
    -it \                           # Start in interactive mode with a virtual terminal
    -v <local-path>:/home/spectre/.spectrecoin/ \ # Mapping of local data folder into the container
    -d \                            # Run in daemon mode
    spectreproject/spectre:latest   # Image to use
```

**IMPORTANT: You must use option _'-v...'_ to mount a host folder into the container, if you use option _'--rm'_! 
Otherwise you might loose your wallet, if the container is stopped!**


To start the container just use

```
docker run --name spectre --rm -it -v ~/.spectrecoin/:/home/spectre/.spectrecoin/ -d spectreproject/spectre:latest
```

#### Stop container
Use cmd _docker stop_ to shutdown a running container. Because of option _--rm_ 
it will be removed automatically afterwards.

```
docker stop <name>
```

#### Restart stopped container
This is only possible, if you do _not_ use option _--rm_ on container start. 
After _docker stop_ the container will stay in place and you can restart it using

```
docker start spectre
```

#### Remove stopped container
This is only possible, if you do _not_ use option _--rm_ on container start. 
After _docker stop_ the container can be removed using

```
docker rm spectre
```

#### Build image yourself
```
docker build -t spectreproject/spectre:latest .
```

#### Using more than one core
If multiple cores available for build, you can pass the amount of cores
to use to the build command:

```
docker build -t spectreproject/spectre:latest --build-arg BUILD_THREADS=12 .
```

Default value is BUILD_THREADS=6
