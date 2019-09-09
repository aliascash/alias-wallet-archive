#!/bin/bash
# ===========================================================================
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
    Checksum-Spectrecoin-CentOS.txt \
    Checksum-Spectrecoin-Debian-Buster.txt \
    Checksum-Spectrecoin-Debian-Stretch.txt \
    Checksum-Spectrecoin-Fedora.txt \
    Checksum-Spectrecoin-Mac.txt \
    Checksum-Spectrecoin-Mac-OBFS4.txt \
    Checksum-Spectrecoin-RaspberryPi-Buster.txt \
    Checksum-Spectrecoin-RaspberryPi-Stretch.txt \
    Checksum-Spectrecoin-Ubuntu.txt \
    Checksum-Spectrecoin-Win64.txt \
    Checksum-Spectrecoin-Win64-OBFS4.txt \
    Checksum-Spectrecoin-Win64-Qt5.9.6.txt \
    Checksum-Spectrecoin-Win64-Qt5.9.6-OBFS4.txt ; do
#    wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/${currentChecksumfile} || true
    wget ${jobURL}/artifact/${currentChecksumfile} || true
    if test -e "${currentChecksumfile}" ; then
        archiveFilename=$(cat ${currentChecksumfile} | cut -d ' ' -f1)
        checksum=$(cat ${currentChecksumfile} | cut -d ' ' -f2)
        echo "**${archiveFilename}:** \`${checksum}\`" >> ${workspace}/releaseNotesToDeploy.txt
        echo '' >> ${workspace}/releaseNotesToDeploy.txt
    fi
done
