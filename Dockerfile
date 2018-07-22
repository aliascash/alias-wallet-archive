### At first perform source build ###
FROM hlxeasy/spectre-builder-part2:latest as build

# Build parameters
ARG BUILD_THREADS="1"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /spectre

RUN cd /spectre \
 && mkdir db4.8 leveldb tor \
 && ./autogen.sh \
 && ./configure \
 && make -j${BUILD_THREADS}
#        --enable-gui \
#        --with-qt5=/usr/include/x86_64-linux-gnu/qt5 \

### Now package binaries into new image ###
FROM hlxeasy/spectre-base:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

COPY --from=build /spectre/src/spectrecoind /usr/local/bin/

USER spectre

CMD ["spectrecoind"]
