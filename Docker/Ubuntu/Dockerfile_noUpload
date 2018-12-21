### At first perform source build ###
FROM spectreproject/spectre-builder-ubuntu:1.5
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="6"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /spectre

RUN cd /spectre \
 && mkdir db4.8 leveldb \
 && ./autogen.sh \
 && ./configure \
        --enable-gui \
 && make -j${BUILD_THREADS}
