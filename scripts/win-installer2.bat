:: SPDX-FileCopyrightText: © 2020 Alias Developers
:: SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
::
:: SPDX-License-Identifier: MIT

IF "%SPECTRECOIN_VERSION%" == "" GOTO NOVERSION
:YESVERSION
IF "%WIX_DIR%" == "" GOTO NOWIX
:YESWIX

set CALL_DIR=%cd%
set SRC_DIR=%cd%\src
set OUT_DIR=%SRC_DIR%\bin
set MSI_TARGET_DIR=%SRC_DIR%\installer
cd

mkdir "%MSI_TARGET_DIR%"

echo on

copy "%SRC_DIR%\qt\res\assets\icons\spectrecoin.ico" "%OUT_DIR%"
cd %MSI_TARGET_DIR%

del Spectrecoin.msi
"%WIX_DIR%\heat.exe" dir "%OUT_DIR%" -dr INSTALLFOLDER -ag **-cg DynamicFragment** -ke -srd -sfrag -sreg -nologo -pog:Binaries -pog:Documents -pog: Satellites -pog:Sources -pog:Content -out "%MSI_TARGET_DIR%\sourceFiles.wxs"

echo "The prepared package is in: %OUT_DIR%"

echo "Everything is OK"
cd %cd%
GOTO END

:NOVERSION
@ECHO The SPECTRECOIN_VERSION environment variable was NOT detected!
GOTO END

:NOWIX
@ECHO The WIX_DIR environment variable was NOT detected!

:END
cd %CALL_DIR%
