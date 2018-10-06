IF "%SPECTRECOIN_VERSION%" == "" GOTO NOVERSION
:YESVERSION

set SRC_DIR=%cd%\src
set OUT_DIR=%SRC_DIR%\Spectrecoin
set MSI_TARGET_DIR=%SRC_DIR%\installer
cd
cd %SRC_DIR%

echo on

copy "%SRC_DIR%\qt\res\assets\icons\spectrecoin.ico" "%OUT_DIR%"
cd %MSI_TARGET_DIR%
del Spectrecoin.msi
C:\devel\mkmsi\mkmsi.py --auto-create qt --source-dir "%OUT_DIR%" --wix-root "C:\Program Files (x86)\WiX Toolset v3.10" --license C:\devel\mkmsi\licenses\GPL3.rtf --merge-module "C:\Program Files (x86)\Common Files\Merge Modules\Microsoft_VC140_CRT_x64.msm" --add-desktop-shortcut --project-version %SPECTRECOIN_VERSION% --description "Spectrecoin - A privacy focused cryptocurrency" --manufacturer "The Spectrecoin Team" Spectrecoin
if %errorlevel% neq 0 exit /b %errorlevel%
copy Spectrecoin.msi %OUT_DIR%\Spectrecoin-%SPECTRECOIN_VERSION%-x64.msi
if %errorlevel% neq 0 exit /b %errorlevel%
copy Spectrecoin.json %OUT_DIR%\Spectrecoin-%SPECTRECOIN_VERSION%-x64.json
if %errorlevel% neq 0 exit /b %errorlevel%
echo "Everything is OK"

echo "The prepared package is in: "%OUT_DIR%"

echo "Everything is OK"


:NOVERSION
@ECHO The SPECTRECOIN_VERSION environment variable was NOT detected!
:END