:: Helper script to create build.h.

set SRC_DIR=%cd%\src
set CALL_DIR=%cd%

cd
cd %SRC_DIR%
dir

echo on

echo "Creating build.h"
FOR /F %DESC in ('git describe --dirty') do echo "#define BUILD_DESC \"%DESC\"" > build.sh
FOR /F %TIME in ('git log -n 1 --format="%ci"') do echo "#define BUILD_DATE \"%TIME\"" >> build.sh

echo "Everything is OK"
cd %CALL_DIR%
