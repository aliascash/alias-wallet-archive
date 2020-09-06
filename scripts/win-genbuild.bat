:: SPDX-FileCopyrightText: © 2020 Alias Developers
:: SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
::
:: SPDX-License-Identifier: MIT
::
:: Helper script to create build.h.
echo off
set SRC_DIR=%cd%\src
set CALL_DIR=%cd%

cd
cd %SRC_DIR%

echo on

@echo Creating build.h

FOR /F "delims=" %%i IN ('git describe --dirty') DO set DESC=%%i
@echo #define BUILD_DESC "%DESC%" > build.h

FOR /F "delims=" %%j IN ('git log -n 1 --format^="%%ci"') DO set TIME=%%j
@echo #define BUILD_DATE "%TIME%" >> build.h

FOR /F "delims=" %%k IN ('git rev-parse --short HEAD') DO set COMMIT=%%k
@echo #define GIT_HASH "%COMMIT%" >> build.h

@echo Created build.h with the following content:
type build.h
cd %CALL_DIR%
