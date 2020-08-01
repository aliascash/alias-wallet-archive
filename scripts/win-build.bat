:: SPDX-FileCopyrightText: © 2020 Alias Developers
:: SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
::
:: SPDX-License-Identifier: MIT
::
:: Helper script to build Aliaswallet on Windows using VS2017 and QT.

IF "%QTDIR%" == "" GOTO NOQT
:YESQT

IF "%VSDIR%" == "" GOTO NOVS
:YESVS

set CALL_DIR=%cd%
set SRC_DIR=%cd%\src
set DIST_DIR=%SRC_DIR%\dist
set BUILD_DIR=%SRC_DIR%\build
set OUT_DIR=%SRC_DIR%\bin
call "%VSDIR%\Community\VC\Auxiliary\Build\vcvars64.bat"
cd
cd %SRC_DIR%
dir

echo on

del "%OUT_DIR%\Alias.exe" 2>nul
rmdir /S /Q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%BUILD_DIR%"
mkdir "%OUT_DIR%"

pushd "%BUILD_DIR%"

%QTDIR%\bin\qmake.exe ^
  -spec win32-msvc ^
  "CONFIG += release" ^
  "%SRC_DIR%\src.pro" || goto :ERROR

nmake || goto :ERROR

popd

%QTDIR%\bin\windeployqt --force --qmldir %SRC_DIR%\qt\res --qml --quick --webengine "%OUT_DIR%\Alias.exe" || goto :ERROR

::ren "%OUT_DIR%" Alias
::echo "The prepared package is in: %SRC_DIR%\Alias"

echo "Everything is OK"
GOTO END

:ERROR
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
GOTO END

:NOVS
@ECHO The VSDIR environment variable was NOT detected!
GOTO END

:NOQT
@ECHO The QTDIR environment variable was NOT detected!

:END
cd %CALL_DIR%
