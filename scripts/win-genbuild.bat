:: Helper script to create build.h.

set SRC_DIR=%cd%\src
set CALL_DIR=%cd%

cd
cd %SRC_DIR%
dir

echo on

echo "Creating build.h"

FOR /F %i in ('git describe --dirty') do set DESC=%i
echo "#define BUILD_DESC \"%DESC%\"" > build.sh

FOR /F %i in ('git log -n 1 --format="%ci"') do set TIME=%i
echo "#define BUILD_DATE \"%TIME%\"" >> build.sh

echo "Everything is OK"
cd %CALL_DIR%
