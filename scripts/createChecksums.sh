#!/usr/bin/env bash
givenFileWithPath=$1
checksumfile=/tmp/checksumfile
if [[ -z "${givenFileWithPath}" ]] ; then
    echo "No filename given, for which checksums should be created!"
    exit 1
fi
if [[ -n "${2}" ]] ; then
    checksumfile=$2
fi
filename=${givenFileWithPath##*/}
echo "${filename}" > ${checksumfile}
echo "md5:       $(md5sum "${givenFileWithPath}" | awk '{ print $1 }')" >> ${checksumfile}
echo "sha1sum:   $(sha1sum "${givenFileWithPath}" | awk '{ print $1 }')" >> ${checksumfile}
echo "sha256sum: $(sha512sum "${givenFileWithPath}" | awk '{ print $1 }')" >> ${checksumfile}
