# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
#
# SPDX-License-Identifier: MIT

### At first perform source build ###
FROM aliascash/alias-wallet-builder-centos-8:1.8 as build
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="6"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /alias-wallet

RUN cd /alias-wallet \
 && mkdir db4.8 leveldb \
 && patch < Docker/Fedora/QT5BinaryPath.patch \
 && ./autogen.sh \
 && ./configure \
        --enable-gui \
        --with-qt5=/usr/include/qt5 \
 && make -j${BUILD_THREADS} \
 && strip /alias-wallet/src/aliaswalletd \
 && strip /alias-wallet/src/aliaswallet

### Now upload binaries to GitHub ###
FROM aliascash/github-uploader:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

ARG GITHUB_TOKEN=1234567
ARG ALIASWALLET_RELEASE=latest
ARG ALIASWALLET_REPOSITORY=alias-wallet
ARG GIT_COMMIT=unknown
ARG REPLACE_EXISTING_ARCHIVE=''
#ENV GITHUB_TOKEN=${GITHUB_TOKEN}
ENV ARCHIVE=Alias-${ALIASWALLET_RELEASE}-${GIT_COMMIT}-CentOS.tgz
ENV CHKSUM_FILE=Checksum-Alias-CentOS.txt

RUN mkdir -p /filesToUpload/usr/local/bin

COPY --from=build /alias-wallet/src/aliaswalletd /filesToUpload/usr/local/bin/
COPY --from=build /alias-wallet/src/aliaswallet /filesToUpload/usr/local/bin/
COPY --from=build /alias-wallet/scripts/createChecksums.sh /tmp/

RUN cd /filesToUpload \
 && tar czf ${ARCHIVE} . \
 && github-release upload \
        --user aliascash \
        --security-token "${GITHUB_TOKEN}" \
        --repo "${ALIASWALLET_REPOSITORY}" \
        --tag "${ALIASWALLET_RELEASE}" \
        --name "${ARCHIVE}" \
        --file "/filesToUpload/${ARCHIVE}" \
        ${REPLACE_EXISTING_ARCHIVE} \
 && chmod +x /tmp/createChecksums.sh \
 && sh /tmp/createChecksums.sh /filesToUpload/${ARCHIVE} ${CHKSUM_FILE} \
 && export GITHUB_TOKEN=---
