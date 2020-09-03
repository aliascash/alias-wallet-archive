#!/bin/bash -e
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# ===========================================================================

ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${ownLocation}"

echo
echo "Nothing to prepare at this stage."
echo
echo "To build, just run one of the cmake-build* scripts on the scripts folder:"
echo
ls -1 scripts/cmake-build*
echo
echo "Use option -h to see their possibilities"
echo
