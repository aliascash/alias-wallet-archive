# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
#
# SPDX-License-Identifier: MIT

ACLOCAL_AMFLAGS = -I m4

if OS_WINDOWS_CROSSCOMPILE
# for leveldb
AM_MAKEFLAGS = TARGET_OS=OS_WINDOWS_CROSSCOMPILE
endif

export CC CXX AR LD
SUBDIRS = db4.8/build_unix leveldb src

Aliaswallet.app: all-am
	mkdir -p Aliaswallet.app/Contents/{MacOS,Resources}
	cp Info.plist Aliaswallet.app/Contents
	cp aliaswallet.icns Aliaswallet.app/Contents/Resources
	cp src/aliaswallet Aliaswallet.app/Contents/MacOS/Aliaswallet
	echo "APPL????" > Aliaswallet.app/Contents/PkgInfo
	@MACDEPLOYQT@ Aliaswallet.app @MACDEPLOYQT_LIBPATH@

.PHONY: Aliaswallet.app
