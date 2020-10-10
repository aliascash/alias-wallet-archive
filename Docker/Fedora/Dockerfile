# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
#
# SPDX-License-Identifier: MIT

### At first perform source build ###
FROM aliascash/alias-wallet-builder-fedora:latest as build
MAINTAINER HLXEasy <hlxeasy@gmail.com>

# Build parameters
ARG BUILD_THREADS="6"

# Runtime parameters
ENV BUILD_THREADS=$BUILD_THREADS

COPY . /alias-wallet

RUN cd /alias-wallet \
 && ./scripts/cmake-build.sh -g -o -s -c ${BUILD_THREADS} \
 && strip /alias-wallet/cmake-build-cmdline/aliaswallet/src/Aliaswalletd \
 && strip /alias-wallet/cmake-build-cmdline/aliaswallet/src/Aliaswallet

### Now upload binaries to GitHub ###
FROM aliascash/github-uploader:latest
MAINTAINER HLXEasy <hlxeasy@gmail.com>

ARG GITHUB_TOKEN=1234567
ARG ALIAS_RELEASE=latest
ARG ALIAS_REPOSITORY=alias-wallet
ARG GIT_COMMIT=unknown
ARG REPLACE_EXISTING_ARCHIVE=''
#ENV GITHUB_TOKEN=${GITHUB_TOKEN}
ENV ARCHIVE=Alias-${ALIAS_RELEASE}-${GIT_COMMIT}-Fedora.tgz
ENV CHKSUM_FILE=Checksum-Alias-Fedora.txt

RUN mkdir -p /filesToUpload/usr/local/bin

COPY --from=build /alias-wallet/cmake-build-cmdline/aliaswallet/src/Aliaswalletd /filesToUpload/usr/local/bin/aliaswalletd
COPY --from=build /alias-wallet/cmake-build-cmdline/aliaswallet/src/Aliaswallet /filesToUpload/usr/local/bin/aliaswallet
COPY --from=build /alias-wallet/scripts/createChecksums.sh /tmp/

RUN cd /filesToUpload \
 && tar czf ${ARCHIVE} . \
 && github-release upload \
        --user aliascash \
        --security-token "${GITHUB_TOKEN}" \
        --repo "${ALIAS_REPOSITORY}" \
        --tag "${ALIAS_RELEASE}" \
        --name "${ARCHIVE}" \
        --file "/filesToUpload/${ARCHIVE}" \
        ${REPLACE_EXISTING_ARCHIVE} \
 && chmod +x /tmp/createChecksums.sh \
 && sh /tmp/createChecksums.sh /filesToUpload/${ARCHIVE} ${CHKSUM_FILE} \
 && export GITHUB_TOKEN=---
