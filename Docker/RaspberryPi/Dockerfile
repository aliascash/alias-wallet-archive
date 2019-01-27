### At first perform source build ###
FROM spectreproject/spectre-builder-raspi:1.4 as build
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

### Now upload binaries to GitHub ###
FROM spectreproject/github-uploader:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

ARG GITHUB_TOKEN=1234567
ARG SPECTRECOIN_RELEASE=latest
ARG SPECTRECOIN_REPOSITORY=spectre
ARG GIT_COMMIT=unknown
ARG REPLACE_EXISTING_ARCHIVE=''
#ENV GITHUB_TOKEN=${GITHUB_TOKEN}
ENV ARCHIVE=Spectrecoin-${SPECTRECOIN_RELEASE}-${GIT_COMMIT}-RaspberryPi.tgz
ENV CHKSUM_FILE=Checksum-Spectrecoin-RaspberryPi.txt

RUN mkdir -p /filesToUpload/usr/local/bin

COPY --from=build /spectre/src/spectrecoind /filesToUpload/usr/local/bin/
COPY --from=build /spectre/src/spectre /filesToUpload/usr/local/bin/spectrecoin
COPY --from=build /spectre/scripts/createChecksums.sh /tmp/

RUN cd /filesToUpload \
 && tar czf ${ARCHIVE} . \
 && github-release upload \
        --user spectrecoin \
        --security-token "${GITHUB_TOKEN}" \
        --repo "${SPECTRECOIN_REPOSITORY}" \
        --tag "${SPECTRECOIN_RELEASE}" \
        --name "${ARCHIVE}" \
        --file "/filesToUpload/${ARCHIVE}" \
        ${REPLACE_EXISTING_ARCHIVE} \
 && chmod +x /tmp/createChecksums.sh \
 && sh /tmp/createChecksums.sh /filesToUpload/${ARCHIVE} ${CHKSUM_FILE} \
 && export GITHUB_TOKEN=---
