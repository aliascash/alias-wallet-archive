### At first perform source build ###
FROM spectreproject/spectre-builder-part2:latest as build
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="1"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /spectre

RUN cd /spectre \
 && mkdir db4.8 leveldb tor \
 && ./autogen.sh \
 && ./configure \
        --enable-gui \
        --with-qt5=/usr/include/x86_64-linux-gnu/qt5 \
 && make -j${BUILD_THREADS}

### Now package binaries into new image ###
FROM spectreproject/spectre-base:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

COPY --from=build /spectre/src/spectrecoind /usr/local/bin/
COPY --from=build /spectre/src/spectre /usr/local/bin/

USER spectre

CMD ["spectrecoind"]
