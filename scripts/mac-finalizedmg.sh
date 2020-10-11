#!/bin/bash
# ===========================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# This script uses macdeployqt to add the required libs to alias package.
# - Fixes non @executable openssl references.
# - Replaces openssl 1.0.0 references with 1.1
#
# ===========================================================================

# Store path from where script was called, determine own location
# and source helper content from there
callDir=$(pwd)
ownLocation="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd ${ownLocation}
. ./include/helpers_console.sh

# Go to Alias repository root directory
cd ..

info "Change permision of .dmg file"
hdiutil convert "Alias.dmg" -format UDRW -o "Alias_Rw.dmg"
info " -> Done"

info "Mount it and save the device"
DEVICE=$(hdiutil attach -readwrite -noverify "Alias_Rw.dmg" |egrep '^/dev/' |sed 1q |awk '{print $1}')
info " -> Done"

sleep 2

info "Create symbolic link to application folder"
PATH_AT_VOLUME=/Volumes/Alias     ## check Path inside cd /Volume/

pushd "$PATH_AT_VOLUME"
ln -s /Applications
popd
info " -> Done"

#info "Copy background image in to package"
#mkdir "$PATH_AT_VOLUME"/.background
#cp backgroundImage.png "$PATH_AT_VOLUME"/.background/
#info " -> Done"

info "Resize window, set background, change icon size, place icons in the right position, etc."
echo '
    tell application "Finder"
    tell disk "Alias"   ## check Path inside cd /Volume/
        open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set the bounds of container window to {400, 100, 1200, 650}
            set viewOptions to the icon view options of container window
            set arrangement of viewOptions to not arranged
            set icon size of viewOptions to 200
            ## set background picture of viewOptions to file ".background:backgroundImage.png"
            set position of item "Alias.app" of container window to {180, 200}
            set position of item "Applications" of container window to {620, 200}
        close
        open
        update without registering applications
        delay 2
    end tell
    end tell
' | osascript
info " -> Done"

sync

info "Unmount"
hdiutil detach "${DEVICE}"
info " -> Done"

info "Cleanup and convert"
rm -f "Alias.dmg"
hdiutil convert "Alias_Rw.dmg" -format UDZO -o "Alias.dmg"
rm -f "Alias_Rw.dmg"
info " -> Done"

info "Finished"

exit
