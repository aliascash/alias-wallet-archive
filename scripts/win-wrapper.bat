:: SPDX-FileCopyrightText: © 2020 Alias Developers
:: SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
::
:: SPDX-License-Identifier: MIT
::
:: Wrapper script to define all requirements

set SPECTRECOIN_VERSION=2.2.0

set QTDIR=C:\Qt\5.9.6\msvc2017_64
set WIX_DIR=C:\Program Files (x86)\WiX Toolset v3.11\bin

call scripts\win-genbuild.bat
call scripts\win-build.bat
::call scripts\win-installer2.bat
