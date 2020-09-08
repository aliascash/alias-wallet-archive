:: ===========================================================================
::
:: SPDX-FileCopyrightText: © 2020 Alias Developers
:: SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
:: SPDX-License-Identifier: MIT
::
:: Created: 2019-10-22 HLXEasy
::
:: Helper script to build Alias on Windows using VS2019 and QT.
::
:: ===========================================================================

IF "%QTDIR%" == "" GOTO NOQT
:YESQT

IF "%VSDIR%" == "" GOTO NOVS
:YESVS

IF "%CMAKEDIR%" == "" GOTO NOCMAKE
:YESVS

IF "%VCPKGDIR%" == "" GOTO NOVCPKG
:YESVS

set SRC_DIR=%cd%
set BUILD_DIR=%cd%\build

:: "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
call "%VSDIR%\Community\VC\Auxiliary\Build\vcvars64.bat"
cd
cd %SRC_DIR%
dir

echo on

rmdir /S /Q "%BUILD_DIR%\delivery"
mkdir "%BUILD_DIR%"
cd %BUILD_DIR%

%CMAKEDIR%\cmake.exe -D CMAKE_TOOLCHAIN_FILE=%VCPKGDIR%\scripts\buildsystems\vcpkg.cmake -D CMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER -D CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER -D ENABLE_GUI=ON -D QT_CMAKE_MODULE_PATH=%QTDIR%\lib\cmake -D CMAKE_BUILD_TYPE=Release .. || goto :ERROR

%CMAKEDIR%\cmake.exe --build . --target Aliaswallet --config Release || goto :ERROR

::ren "%OUT_DIR%" Alias
::echo "The prepared package is in: %BUILD_DIR%\delivery"

echo "Everything is OK"
GOTO END

:ERROR
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
GOTO END

:NOVCPKG
@ECHO The VCPKGDIR environment variable was NOT detected!
GOTO END

:NOCMAKE
@ECHO The CMAKEDIR environment variable was NOT detected!
GOTO END

:NOVS
@ECHO The VSDIR environment variable was NOT detected!
GOTO END

:NOQT
@ECHO The QTDIR environment variable was NOT detected!

:END
cd %SRC_DIR%
