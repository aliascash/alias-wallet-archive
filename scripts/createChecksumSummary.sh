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
    echo "${releaseDescription}" > ${workspace}/releaseNotesToDeploy.txt
fi
for currentChecksumfile in Checksum-Spectrecoin-CentOS.txt Checksum-Spectrecoin-Debian.txt Checksum-Spectrecoin-Fedora.txt Checksum-Spectrecoin-Mac.txt Checksum-Spectrecoin-OBFS4-Mac.txt Checksum-Spectrecoin-OBFS4-WIN64.txt Checksum-Spectrecoin-Qt5.12-OBFS4-WIN64.txt Checksum-Spectrecoin-Qt5.12-WIN64.txt Checksum-Spectrecoin-RaspberryPi.txt Checksum-Spectrecoin-Ubuntu.txt Checksum-Spectrecoin-WIN64.txt ; do
#    wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/${currentChecksumfile} || true
    wget ${jobURL}/artifact/${currentChecksumfile} || true
    if test -e "${currentChecksumfile}" ; then
        cat "${currentChecksumfile}" >> ${workspace}/releaseNotesToDeploy.txt
    fi
done
