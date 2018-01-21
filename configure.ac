AC_INIT([spectrecoind], [1.3.5])
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
AX_CXX_COMPILE_STDCXX(14, noext, mandatory)
AC_CONFIG_FILES([Makefile src/Makefile])
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(OPENSSL, [openssl < 1.1], [], [AC_MSG_ERROR([OpenSSL 1.0 not found])])
PKG_CHECK_MODULES(LIBEVENT, [libevent], [], [AC_MSG_ERROR([libevent not found])])
AX_SUBDIRS_CONFIGURE([tor], [[--disable-unittests], [--disable-system-torrc], [--disable-systemd], [--disable-lzma], [--disable-asciidoc], [--with-openssl-dir=$($PKG_CONFIG --variable=libdir openssl)], [CFLAGS=$($PKG_CONFIG --cflags openssl) -O2]])
if test "x$build_windows" = "xyes" ; then
  AX_SUBDIRS_CONFIGURE([db4.8/build_unix], [[--disable-cryptography], [--disable-partition], [--disable-compression], [--disable-replication], [--enable-cxx], [--enable-static], [--disable-shared], [--enable-mingw], [CFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1], [CPPFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1]])
else
  AX_SUBDIRS_CONFIGURE([db4.8/build_unix], [[--disable-cryptography], [--disable-partition], [--disable-compression], [--disable-replication], [--enable-cxx], [--enable-static], [--disable-shared], [CFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1], [CPPFLAGS=-fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1]])
fi
PKG_CHECK_MODULES(ZLIB, [zlib], [], [AC_MSG_ERROR([zlib not found])])
PKG_CHECK_MODULES(ZSTD, [libzstd], [], [AC_MSG_NOTICE([libzstd not found])])
AC_CHECK_LIB([pthread], [pthread_self], , [AC_MSG_ERROR([libpthread not found])])
AC_LANG_CPLUSPLUS
#AC_CHECK_LIB([snappy], [main])
BOOST_REQUIRE([1.58])
BOOST_SYSTEM([mt])
BOOST_CHRONO([mt])
BOOST_FILESYSTEM([mt])
BOOST_PROGRAM_OPTIONS([mt])
BOOST_THREAD([mt])
BOOST_ASIO
BOOST_DATE_TIME([mt])
BOOST_IOSTREAMS([mt])
AC_ARG_ENABLE([gui], [AS_HELP_STRING([--enable-gui], [Build the Qt GUI wallet])])
if test "x$enable_gui" = "xyes" ; then
  PKG_CHECK_MODULES(QT5, [Qt5Core Qt5Gui Qt5Widgets], [], [AC_MSG_ERROR([Qt5 not found.])])
  PKG_CHECK_MODULES(QT5WEBKIT, [Qt5WebKit Qt5WebKitWidgets], [], [AC_MSG_ERROR([Qt5WebKit not found.])])
  beginswith() { case $2 in "$1"*) true;; *) false;; esac; }
  AC_CHECK_TOOLS(MOC, [moc-qt5 moc])
  if beginswith "$($MOC -v)" "$MOC 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "moc" - please install the Qt5 development tools.])
  fi
  AC_CHECK_TOOLS(UIC, [uic-qt5 uic])
  if beginswith "$($UIC -v)" "$UIC 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "uic" - please install the Qt5 development tools.])
  fi
  AC_CHECK_TOOLS(RCC, [rcc-qt5 rcc])
  if beginswith "$($RCC -v)" "$RCC 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "rcc" - please install the Qt5 development tools.])
  fi
  AC_CHECK_TOOLS(LRELEASE, [lrelease-qt5 lrelease])
  if beginswith "$($LRELEASE -version)" "lrelease version 5." ; then
    AC_MSG_ERROR([Cannot find Qt5 tool "lrelease" - please install the Qt5 development tools.])
  fi
fi
AM_CONDITIONAL([ENABLE_GUI], [test "$enable_gui" = "yes"])
AC_OUTPUT
