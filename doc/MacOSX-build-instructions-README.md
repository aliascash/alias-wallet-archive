# Aliaswallet building from source for Mac OSX

At first you need to clone the Git repository. ;-)

## Build with Qt-Creator
### Install Qt SDK 5.12.2 (QtWebEngine)
- Qt SDK: https://www.qt.io/download-qt-installer

Now you can open `<path-to-your-alias-wallet-git-repo-clone>/src/src.pro` on Qt-Creator.


## Build on cmdline
- Export path to Qt
```
export QT_PATH=~/Qt/5.12.2/clang_64
```

### Setup required libraries and env var's
#### Boost
- Determine amount of available cores to improve build speed
```
system_profiler | grep "Total Number of Cores"
export CORES=<insert-value-from-above-cmd>
```
- Download, extract and build Boost
```
cd ~
mkdir Boost
cd Boost
wget https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
tar xzf boost_1_68_0.tar.gz
cd boost_1_68_0
./bootstrap.sh
./b2 \
    cxxflags="-std=c++0x" \
    address-model=64 \
    -j ${CORES}\
    install \
    --prefix=$(pwd) \
    --build-type=complete \
    --layout=tagged
```
- Export path to Boost libraries and headers
```
export BOOST_PATH=$(pwd)
```

#### OpenSSL
- Install OpenSSL 1.1.1
- Export path to OpenSSL libraries and headers i. e. like this:
```
export OPENSSL_PATH=/usr/local/Cellar/openssl@1.1/1.1.1d
```

#### Tor
- Download prepared Tor archive
```
cd ~
mkdir Tor
cd Tor
wget https://github.com/aliascash/resources/raw/master/resources/Aliaswallet.Tor.libraries.macOS.zip
```

### Build using helper scripts

```
cd <path-to-your-alias-wallet-git-repo-clone>
./scripts/mac-build.sh
rm -f Aliaswallet*.dmg
unzip ~/Tor/Tor.zip
# rm -rf src/bin/debug
./scripts/mac-deployqt.sh
```
