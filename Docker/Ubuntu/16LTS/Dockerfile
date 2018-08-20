### At first perform source build ###
FROM spectreproject/spectre-builder-ubuntu-16LTS:latest as build
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="6"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /spectre

RUN cd /spectre \
 && mkdir db4.8 leveldb tor \
 && ./autogen.sh \
 && ./configure \
        --enable-gui \
 && make -j${BUILD_THREADS}

### Now package binaries into new image ###
FROM spectreproject/spectre-base-ubuntu-16LTS:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

COPY --from=build /spectre/src/spectrecoind /usr/local/bin/
COPY --from=build /spectre/src/spectre /usr/local/bin/spectrecoin

USER spectre
