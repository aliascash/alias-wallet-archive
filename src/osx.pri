# 1. Before using autotools & make, make sure to set the macOS target platform in the shell:
# export MACOSX_DEPLOYMENT_TARGET=10.10

# 2. Install all libraries with homebrew:
# brew install autoconf automake libtool pkg-config openssl@1.1 libevent boost gcc wget

# (2a optional) Fix for xCode to stop complaining about not finding the string.h file
# https://stackoverflow.com/questions/48839127/qmake-derived-clang-in-osx-10-13-cannot-find-string-h
# INCLUDEPATH += /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/usr/include

ICON = $$PWD/qt/res/assets/icons/spectre.icns

    # to build tor with autotools, call in subfolder tor:
    # ./autogen.sh && ./configure --with-ssl-dir=/usr/local/Cellar/openssl@1.1/1.1.1 --disable-asciidoc --disable-lzma
#    LIBS += -L$$PWD/../tor/src/or -ltor \
#    -L$$PWD/../tor/src/common -lor \
#    -L$$PWD/../tor/src/common -lor-ctime \
#    -L$$PWD/../tor/src/common -lor-crypto \
#    -L$$PWD/../tor/src/common -lor-event \
#    -L$$PWD/../tor/src/trunnel -lor-trunnel \
#    -L$$PWD/../tor/src/common -lcurve25519_donna \
#    -L$$PWD/../tor/src/ext/ed25519/donna -led25519_donna \
#    -L$$PWD/../tor/src/ext/ed25519/ref10 -led25519_ref10 \
#    -L$$PWD/../tor/src/ext/keccak-tiny -lkeccak-tiny \

#    INCLUDEPATH += $$PWD/../tor/src/or $$PWD/../tor/src/common $$PWD/../tor/src/trunnel $$PWD/../tor/src/ext/ed25519/donna $$PWD/../tor/src/ext/ed25519/ref10 $$PWD/../tor/src/ext/keccak-tiny
#    DEPENDPATH += $$PWD/../tor/src/or $$PWD/../tor/src/common $$PWD/../tor/src/trunnel $$PWD/../tor/src/ext/ed25519/donna $$PWD/../tor/src/ext/ed25519/ref10 $$PWD/../tor/src/ext/keccak-tiny

#    PRE_TARGETDEPS += $$PWD/../tor/src/or/libtor.a \
#    $$PWD/../tor/src/common/libor.a \
#    $$PWD/../tor/src/common/libor-ctime.a \
#    $$PWD/../tor/src/common/libor-crypto.a \
#    $$PWD/../tor/src/common/libor-event.a \
#    $$PWD/../tor/src/trunnel/libor-trunnel.a \
#    $$PWD/../tor/src/common/libcurve25519_donna.a \
#    $$PWD/../tor/src/ext/ed25519/donna/libed25519_donna.a \
#    $$PWD/../tor/src/ext/ed25519/ref10/libed25519_ref10.a \
#    $$PWD/../tor/src/ext/keccak-tiny/libkeccak-tiny.a \

    # brew install zlib
    _ZLIB_PATH = /usr/local/opt/zlib
    INCLUDEPATH += "$${_ZLIB_PATH}/include/"
    LIBS += -L$${_ZLIB_PATH}/lib
    #Shblis-MacBook-Pro:src Shbli$ find /usr/local/Cellar/libevent/2.1.8/lib/ -name *a
    LIBS += -lz

    # to build BerkeleyDB with autotools, call in subfolder db4.8:
    # ./configure --enable-cxx --disable-shared --disable-replication --with-pic && make
    _BERKELEY_PATH = $$PWD/../db4.8/build_unix
    LIBS += $${_BERKELEY_PATH}/libdb_cxx.a \ # link static
    $${_BERKELEY_PATH}/libdb.a

    DEPENDPATH += $${_BERKELEY_PATH}
    INCLUDEPATH += $${_BERKELEY_PATH}

    # to build leveldb call in subfolder leveldb:
    # ./build_detect_platform build_config.mk ./ && make
    LIBS += -L$$PWD/../leveldb/out-static -lleveldb \
    -L$$PWD/../leveldb/out-static -lmemenv

    INCLUDEPATH += $$PWD/../leveldb/out-static
    DEPENDPATH += $$PWD/../leveldb/out-static
    INCLUDEPATH += $$PWD/../leveldb/include
    DEPENDPATH += $$PWD/../leveldb/include
    INCLUDEPATH += $$PWD/../leveldb/helpers
    DEPENDPATH += $$PWD/../leveldb/helpers

    PRE_TARGETDEPS += $$PWD/../leveldb/out-static/libleveldb.a \
    $$PWD/../leveldb/out-static/libmemenv.a


HEADERS +=              $$PWD/qt/macdockiconhandler.h \
                        $$PWD/qt/macnotificationhandler.h \

OBJECTIVE_SOURCES +=    $$PWD/qt/macdockiconhandler.mm \
                        $$PWD/qt/macnotificationhandler.mm \

DEFINES += FORTIFY_SOURCE=1

QMAKE_LFLAGS += -fstack-protector
QMAKE_CXXFLAGS += -pthread -fPIC -fstack-protector -O2 -D_FORTIFY_SOURCE=1 -Wall -Wextra -Wno-ignored-qualifiers -Woverloaded-virtual -Wformat -Wformat-security -Wno-unused-parameter

DEFINES += MAC_OSX
DEFINES += HAVE_BUILD_INFO
# Mac: compile for maximum compatibility (10.0 Yosemite, 32-bit)
QMAKE_CXXFLAGS += -std=c++17 -mmacosx-version-min=10.10 -isysroot

    # https://www.reddit.com/r/cpp/comments/334s4r/how_to_enable_c14_in_qt_creator_on_a_mac/
    # TODO might be obsolete with C++17: Turns out, there's a glitch in the Mac version where the standard library isn't correctly included when using the C++14 config flag. Adding this additional line to the .pro file fixes the problem:
    QMAKE_CXXFLAGS += -stdlib=libc++

    #add in core foundation framework
    QMAKE_LFLAGS += -F /System/Library/Frameworks/CoreFoundation.framework/
    LIBS += -framework CoreFoundation

    QMAKE_LFLAGS += -F /System/Library/Frameworks/AppKit.framework/
    LIBS += -framework AppKit

    QMAKE_LFLAGS += -F /System/Library/Frameworks/ApplicationServices.framework/
    LIBS += -framework ApplicationServices


    #brew install boost
    # $BOOST_PATH is set via environment
#    _BOOST_PATH = /usr/local/Cellar/boost/1.68.0_1
#    BOOST_PATH = /usr/local/Cellar/boost/1.68.0_1
    INCLUDEPATH += "$${BOOST_PATH}/include/"
    LIBS += -L$${BOOST_PATH}/lib
    LIBS += -lboost_system-mt -lboost_chrono-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt -lboost_unit_test_framework-mt -lboost_timer-mt # using dynamic lib (not sure if you need that "-mt" at the end or not)
    DEFINES += BOOST_ASIO_ENABLE_OLD_SERVICES BOOST_SPIRIT_THREADSAFE BOOST_THREAD_USE_LIB BOOST_THREAD_POSIX BOOST_HAS_THREADS
    #LIBS += $${BOOST_PATH}/lib/libboost_chrono-mt.a # using static lib

    #brew install openssl@1.1
    # $OPENSSL_PATH is set via environment
#    _OPENSSL_PATH = /usr/local/Cellar/openssl@1.1/1.1.1
#    OPENSSL_PATH = /usr/local/Cellar/openssl@1.1/1.1.0h
    # See http://doc.qt.io/archives/qt-4.8/qmake-advanced-usage.html#variables
    INCLUDEPATH += "${OPENSSL_PATH}/include/"
    LIBS += -L${OPENSSL_PATH}/lib
    LIBS += -lssl -lcrypto # using dynamic lib (not sure if you need that "-mt" at the end or not)

    #libevent-2.1.6.dylib
    #brew install libevent
    _LIBEVENT_PATH = /usr/local/Cellar/libevent/2.1.8
    INCLUDEPATH += "$${_LIBEVENT_PATH}/include/"
    LIBS += -L$${_LIBEVENT_PATH}/lib
    #Shblis-MacBook-Pro:src Shbli$ find /usr/local/Cellar/libevent/2.1.8/lib/ -name *a
    LIBS += -levent_extra -levent -levent_core -levent_openssl -levent_pthreads
