:: Helper script to build Spectrecoin on Windows using VS2017 and QT.

IF "%QTDIR%" == "" GOTO NOQT
:YESQT

set CALL_DIR=%cd%
set SRC_DIR=%cd%\src
set DIST_DIR=%SRC_DIR%\dist
set BUILD_DIR=%SRC_DIR%\build
set OUT_DIR=%SRC_DIR%\bin
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
cd
cd %SRC_DIR%
dir

echo on

del "%OUT_DIR%\Spectrecoin.exe" 2>nul
rmdir /S /Q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%BUILD_DIR%"
mkdir "%OUT_DIR%"

pushd "%BUILD_DIR%"

%QTDIR%\bin\qmake.exe ^
  -spec win32-msvc ^
  "CONFIG += release" ^
  "%SRC_DIR%\src.pro"

nmake

popd

%QTDIR%\bin\windeployqt "%OUT_DIR%\Spectrecoin.exe"

::ren "%OUT_DIR%" Spectrecoin
::echo "The prepared package is in: %SRC_DIR%\Spectrecoin"

echo "Everything is OK"
GOTO END

:NOQT
@ECHO The QTDIR environment variable was NOT detected!

:END
cd %CALL_DIR%
