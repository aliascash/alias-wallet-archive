### At first perform source build ###
FROM spectreproject/spectre-builder-fedora:latest as build
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="6"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /spectre

RUN cd /spectre \
 && mkdir db4.8 leveldb tor \
 && patch < Docker/Fedora/QT5BinaryPath.patch \
 && ./autogen.sh \
 && ./configure \
        --enable-gui \
        --with-qt5=/usr/include/qt5 \
 && make -j${BUILD_THREADS}

### Now package binaries into new image ###
FROM spectreproject/spectre-base-fedora:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

COPY --from=build /spectre/src/spectrecoind /usr/local/bin/
COPY --from=build /spectre/src/spectre /usr/local/bin/spectrecoin

USER spectre
