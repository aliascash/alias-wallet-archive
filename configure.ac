# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
#
# SPDX-License-Identifier: MIT

AC_INIT([aliaswalletd], [1.4.1])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign 1.13 subdir-objects -Wall -Werror -Wno-portability])
AC_CONFIG_MACRO_DIRS([m4])
AM_PROG_AS
AM_PROG_AR
AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_CXX
AC_PROG_OBJCXX
AC_PROG_LIBTOOL
AC_CANONICAL_HOST
build_linux=no
build_windows=no
build_mac=no
case "${host_os}" in
    linux*)
        build_linux=yes
        ;;
    cygwin*|mingw*)
        build_windows=yes
        ;;
    darwin*)
        build_mac=yes
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported"])
        ;;
esac
AM_CONDITIONAL([OS_LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([OS_WINDOWS_CROSSCOMPILE], [test "$build_windows" = "yes"])
AM_CONDITIONAL([OS_MACOS], [test "$build_mac" = "yes"])
AX_CXX_COMPILE_STDCXX(17, noext, mandatory)
AC_CONFIG_FILES([Makefile src/Makefile])
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(OPENSSL, [openssl >= 1.1], [], [AC_MSG_ERROR([OpenSSL 1.1 not found])])
PKG_CHECK_MODULES(LIBEVENT, [libevent], [], [AC_MSG_ERROR([libevent not found])])
#AX_SUBDIRS_CONFIGURE([tor], [[--disable-unittests], [--disable-system-torrc], [--disable-systemd], [--disable-asciidoc], [--with-openssl-dir=$($PKG_CONFIG --variable=libdir openssl)], [CFLAGS=$($PKG_CONFIG --cflags openssl) -O2]])
if test "x$build_windows" = "xyes" ; then
  AX_SUBDIRS_CONFIGURE([db4.8/build_unix], [[--disable-cryptography], [--disable-partition], [--disable-compression], [--disable-replication], [--enable-cxx], [--enable-static], [--disable-shared], [--enable-mingw], [CFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1], [CPPFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1]])
else
  AX_SUBDIRS_CONFIGURE([db4.8/build_unix], [[--disable-cryptography], [--disable-partition], [--disable-compression], [--disable-replication], [--enable-cxx], [--enable-static], [--disable-shared], [CFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1], [CPPFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1]])
fi
PKG_CHECK_MODULES(ZLIB, [zlib], [], [AC_MSG_ERROR([zlib not found])])
PKG_CHECK_MODULES(ZSTD, [libzstd], [], [AC_MSG_NOTICE([libzstd not found])])
PKG_CHECK_MODULES(LZMA, [liblzma], [], [AC_MSG_NOTICE([liblzma not found])])
AC_CHECK_LIB([snappy], [main], , [AC_MSG_NOTICE([snappy not found])])
AC_CHECK_LIB([pthread], [pthread_self], , [AC_MSG_ERROR([libpthread not found])])
AC_LANG_CPLUSPLUS
# TODO investigate whether earlier Boost versions will still work.
BOOST_REQUIRE([1.62])
BOOST_SYSTEM([mt])
BOOST_CHRONO([mt])
BOOST_FILESYSTEM([mt])
BOOST_PROGRAM_OPTIONS([mt])
BOOST_THREAD([mt])
BOOST_ASIO
BOOST_DATE_TIME([mt])
BOOST_IOSTREAMS([mt])
MACOS_RPATH=""
AC_ARG_ENABLE([gui], [AS_HELP_STRING([--enable-gui], [Build the Qt GUI wallet])])
if test "x$enable_gui" = "xyes" ; then
  AC_ARG_WITH([qt5], [AS_HELP_STRING([--with-qt5], [Manually specify the path to a stand-alone Qt5 installation (pkg-config will be used if not set). This has no effect if --enable-gui is not also specified.])], [
    if test "x$build_mac" = "xyes" ; then
      QT5_LIBDIR="$with_qt5/lib"
      MACOS_RPATH="-Wl,-rpath,$QT5_LIBDIR"
      QT5_LDFLAGS="-F$QT5_LIBDIR"
      QT5_CFLAGS="$QT5_LDFLAGS -I$QT5_LIBDIR/QtCore.framework/Headers -I$QT5_LIBDIR/QtGui.framework/Headers -I$QT5_LIBDIR/QtWidgets.framework/Headers -I$QT5_LIBDIR/QtNetwork.framework/Headers -I$QT5_LIBDIR/QtWebChannel.framework/Headers -I$QT5_LIBDIR/QtQuick.framework/Headers -I$QT5_LIBDIR/QtQuickWidgets.framework/Headers -I$QT5_LIBDIR/QtWebView.framework/Headers -I$QT5_LIBDIR/QtWebSockets.framework/Headers -I$QT5_LIBDIR/QtSvg.framework/Headers"
      QT5_LIBS="-framework QtCore -framework QtGui -framework QtWidgets -framework QtNetwork -framework QtWebChannel -framework QtQuick -framework QtQuickWidgets -framework QtWebView -framework QtWebSockets -framework QtSvg"

      MACDEPLOYQT="$with_qt5/bin/macdeployqt"
      AC_SUBST([MACDEPLOYQT])
    else
      QT5_CFLAGS="-I$with_qt5 -I$with_qt5/QtGui -I$with_qt5/QtWidgets -I$with_qt5/QtCore -I$with_qt5/QtWebChannel -I$with_qt5/QtNetwork -I$with_qt5/QtQuick -I$with_qt5/QtQuickWidgets -I$with_qt5/QtWebView -I$with_qt5/QtWebSockets -I$with_qt5/QtSvg"
      QT5_LDFLAGS="-L$with_qt5/lib"
      QT5_LIBS="-lQt5Core -lQt5Gui -lQt5Widgets -lQt5Network -lQt5WebChannel -lQt5Quick -lQt5QuickWidgets -lQt5WebView -lQt5WebSockets -lQt5Svg"
    fi
    AC_SUBST([QT5_CFLAGS])
    AC_SUBST([QT5_LDFLAGS])
    AC_SUBST([QT5_LIBS])
    MOC="${with_qt5//\/include\//\/lib\//}/bin/moc"
    UIC="${with_qt5//\/include\//\/lib\//}/bin/uic"
    RCC="${with_qt5//\/include\//\/lib\//}/bin/rcc"
    LRELEASE="${with_qt5//\/include\//\/lib\//}/bin/lrelease"
    AC_SUBST([MOC])
    AC_SUBST([UIC])
    AC_SUBST([RCC])
    AC_SUBST([LRELEASE])
    AC_MSG_NOTICE([Using Qt5 at $with_qt5])
  ], [
    PKG_CHECK_MODULES(QT5, [Qt5Core Qt5Gui Qt5Widgets Qt5Network Qt5WebChannel Qt5Quick Qt5QuickWidgets Qt5WebView Qt5WebSockets Qt5Svg], [AC_MSG_NOTICE([Found Qt5 using pkg-config.])], [
      PKG_CHECK_MODULES(QT5, [QtCore >= 5 QtGui >= 5 QtWidgets >= 5 QtNetwork >= 5 QtWebChannel >= 5 QtQuick >= 5 QtQuickWidgets >= 5 QtWebView >= 5 QtWebSockets >= 5 QtSvg >= 5], [AC_MSG_NOTICE([Found Qt >= 5 using pkg-config.])], [
        AC_MSG_ERROR([Qt5 not found.])
      ])
    ])
  ])
  MACDEPLOYQT_LIBPATH=""
  AC_SUBST([MACDEPLOYQT_LIBPATH])
  notbeginswith() { case $2 in "$1"*) true;; *) false;; esac; }
  if test "x$MOC" = "x" ; then
    AC_CHECK_TOOLS(MOC, [moc-qt5 moc])
  fi
  if notbeginswith "$($MOC -v)" "$MOC 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "moc" - please install the Qt5 development tools.])
  fi
  if test "x$UIC" = "x" ; then
    AC_CHECK_TOOLS(UIC, [uic-qt5 uic])
  fi
  if notbeginswith "$($UIC -v)" "$UIC 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "uic" - please install the Qt5 development tools.])
  fi
  if test "x$RCC" = "x" ; then
    AC_CHECK_TOOLS(RCC, [rcc-qt5 rcc])
  fi
  if notbeginswith "$($RCC -v)" "$RCC 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "rcc" - please install the Qt5 development tools.])
  fi
  if test "x$LRELEASE" = "x" ; then
    AC_CHECK_TOOLS(LRELEASE, [lrelease-qt5 lrelease])
  fi
  if notbeginswith "$($LRELEASE -version)" "lrelease version 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "lrelease" - please install the Qt5 translation tools.])
  fi
  if test "x$build_mac" = "xyes" ; then
    if test "x$MACDEPLOYQT" = "x" ; then
      AC_CHECK_TOOLS(MACDEPLOYQT, [macdeployqt-qt5 macdeployqt])
    fi
  fi
fi
CPPFLAGS="$CPPFLAGS -DHAVE_BUILD_INFO"
AC_SUBST([MACOS_RPATH])
AM_CONDITIONAL([ENABLE_GUI], [test "$enable_gui" = "yes"])
AC_OUTPUT
