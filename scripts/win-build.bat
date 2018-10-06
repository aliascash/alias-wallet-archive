set SRC_DIR=%cd%\\src
set DIST_DIR=%SRC_DIR%\\dist
set BUILD_DIR=%SRC_DIR%\\build
set MSI_TARGET_DIR=C:\\jenkins\\build\\whid-x64
call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"
cd
cd %SRC_DIR%
dir

echo on

rmdir /S /Q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%BUILD_DIR%"
mkdir "%OUT_DIR%"

pushd "%BUILD_DIR%"

%QTDIR%\\bin\\qmake.exe ^
  -spec win32-msvc ^
  "CONFIG += release" ^
  "%SRC_DIR%\\src.pro"

nmake

popd

echo "Copying: %BUILD_DIR%\\release\\Spectrecoin.exe" "%OUT_DIR%"
copy "%BUILD_DIR%\\release\\Spectrecoin.exe" "%OUT_DIR%"
copy "%SRC_DIR%\\qt\\res\\assets\\icons\\spectrecoin.ico" "%OUT_DIR%"

%QTDIR%\\bin\\windeployqt "%OUT_DIR%\\Spectrecoin.exe"

echo "The prepared package is in: "%OUT_DIR%"

echo "Everything is OK"
