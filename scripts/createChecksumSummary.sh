#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Created: 2019-01-22 HLXEasy
#
# Helper script to create checksums for given file (1st parameter)
# and write them into given another file (2nd parameter)
#
# ===========================================================================

releaseDescription=$1
workspace=$2
jobURL=$3

if test -e "${releaseDescription}" ; then
    cp "${releaseDescription}" ${workspace}/releaseNotesToDeploy.txt
else
    echo "### ${releaseDescription}" > ${workspace}/releaseNotesToDeploy.txt
fi
for currentChecksumfile in \
    Checksum-Aliaswallet-CentOS.txt \
    Checksum-Aliaswallet-Debian-Buster.txt \
    Checksum-Aliaswallet-Debian-Stretch.txt \
    Checksum-Aliaswallet-Fedora.txt \
    Checksum-Aliaswallet-Mac.txt \
    Checksum-Aliaswallet-Mac-OBFS4.txt \
    Checksum-Aliaswallet-RaspberryPi-Buster.txt \
    Checksum-Aliaswallet-RaspberryPi-Stretch.txt \
    Checksum-Aliaswallet-Ubuntu-18-04.txt \
    Checksum-Aliaswallet-Ubuntu-19-04.txt \
    Checksum-Aliaswallet-Ubuntu-19-10.txt \
    Checksum-Aliaswallet-Win64.txt \
    Checksum-Aliaswallet-Win64-OBFS4.txt \
    Checksum-Aliaswallet-Win64-Qt5.12.txt \
    Checksum-Aliaswallet-Win64-Qt5.12-OBFS4.txt \
    Checksum-Aliaswallet-Win64-Qt5.9.6.txt \
    Checksum-Aliaswallet-Win64-Qt5.9.6-OBFS4.txt ; do
#    wget https://ci.alias.cash/job/Alias/job/aliaswallet/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/${currentChecksumfile} || true
    wget ${jobURL}/artifact/${currentChecksumfile} || true
    if test -e "${currentChecksumfile}" ; then
        archiveFilename=$(cat ${currentChecksumfile} | cut -d ' ' -f1)
        checksum=$(cat ${currentChecksumfile} | cut -d ' ' -f2)
        echo "**${archiveFilename}:** \`${checksum}\`" >> ${workspace}/releaseNotesToDeploy.txt
        echo '' >> ${workspace}/releaseNotesToDeploy.txt
    fi
done
