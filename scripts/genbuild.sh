#!/bin/sh
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# ===========================================================================

if [ $# -gt 0 ] ; then
    FILE="$1"
    shift
    if [ -f "$FILE" ] ; then
        INFO="$(head -n 1 "$FILE")"
    fi
else
    echo "Usage: $0 <filename>"
    exit 1
fi

if [ -e "$(which git)" ] ; then
    # clean 'dirty' status of touched files that haven't been modified
    git diff >/dev/null 2>/dev/null

    # get a string like "v0.6.0-66-g59887e8-dirty"
    DESC="$(git describe --dirty 2>/dev/null)"

    # get a string like "2012-04-10 16:27:19 +0200"
    TIME="$(git log -n 1 --format="%ci")"

    # get short commit hash
    COMMIT_ID="$(git rev-parse --short HEAD)"
fi

if [ -n "$DESC" ] ; then
    NEWINFO="#define BUILD_DESC \"$DESC\""
else
    NEWINFO="// No build information available"
fi

# only update build.h if necessary
if [ "$INFO" != "$NEWINFO" ] ; then
    echo "$NEWINFO" >"$FILE"
    echo "#define BUILD_DATE \"$TIME\"" >>"$FILE"
    echo "#define GIT_HASH \"$COMMIT_ID\"" >>"$FILE"
fi
