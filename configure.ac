AC_INIT([spectrecoind], [1.3.4])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign 1.11 subdir-objects -Wall -Werror -Wno-portability])
AM_PROG_AS
AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_CXX
AC_CONFIG_FILES([Makefile src/Makefile])
PKG_CHECK_MODULES(OPENSSL, [openssl < 1.1], [], [AC_MSG_ERROR([OpenSSL 1.0 not found])])
AX_SUBDIRS_CONFIGURE([tor], [[--disable-unittests], [--disable-system-torrc], [--disable-systemd], [--disable-lzma], [--disable-asciidoc], [--with-openssl-dir=$(pkg-config --variable=libdir openssl)], [CFLAGS=$(pkg-config --cflags openssl)]])
AX_SUBDIRS_CONFIGURE([db4.8/build_unix], [[--disable-cryptography], [--disable-partition], [--disable-compression], [--disable-replication], [--enable-cxx], [--enable-static], [--disable-shared]])
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(LIBEVENT, [libevent], [], [AC_MSG_ERROR([libevent not found])])
PKG_CHECK_MODULES(LIBSECCOMP, [libseccomp], [], [AC_MSG_ERROR([libseccomp not found])])
PKG_CHECK_MODULES(LIBCAP, [libcap], [], [AC_MSG_ERROR([libcap not found])])
PKG_CHECK_MODULES(ZLIB, [zlib], [], [AC_MSG_ERROR([zlib not found])])
AC_CHECK_LIB([pthread], [pthread_self], , [AC_MSG_ERROR([libpthread not found.])])
AC_LANG_CPLUSPLUS
AC_CHECK_LIB([snappy], [main])
AC_CHECK_LIB([boost_system], [main], ,
             [AC_MSG_ERROR([libboost_system not found.])])
AC_CHECK_LIB([boost_filesystem], [main], ,
             [AC_MSG_ERROR([libboost_filesystem not found.])])
AC_CHECK_LIB([boost_program_options], [main], ,
             [AC_MSG_ERROR([libboost_program_options not found.])])
AC_CHECK_LIB([boost_thread], [main], ,
             [AC_MSG_ERROR([libboost_thread not found.])])
AC_ARG_ENABLE([gui], [AS_HELP_STRING([--enable-gui], [Build the Qt GUI wallet])])
if test "$enable_gui" = "yes" ; then
  PKG_CHECK_MODULES(QT4, [QtCore QtGui QtWebKit >= 4.4.0], [], [AC_MSG_ERROR([Qt4 not found.])])
  if ! `$PKG_CONFIG --atleast-version=4.8.0 QtCore`; then
   AC_MSG_ERROR([Qt4 >= 4.8 not found.])
  fi
  AC_CHECK_PROGS(MOC, [moc-qt4 moc])
  AC_CHECK_PROGS(UIC, [uic-qt4 uic])
  AC_CHECK_PROGS(RCC, [rcc-qt4 rcc])
  AC_CHECK_PROGS(LRELEASE, [lrelease-qt4 lrelease])
  if test -z "$MOC" || test -z "$UIC" || test -z "$RCC" || test -z "$LRELEASE"; then
    AC_MSG_ERROR([Qt utilities moc/uic/rcc/lrelease not found.])
  fi
fi
AM_CONDITIONAL([ENABLE_GUI], [test "$enable_gui" = "yes"])
AC_OUTPUT
