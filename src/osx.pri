#TOR LIBS, to build tor use autotools/autoconfig in tor subfolder, make sure to pass -with-openssl 1.1 and pass the directory there
#Note, to compile the tor pojrect use the following command
#./configure --enable-static-tor --with-openssl-dir=/usr/local/Cellar/openssl@1.1/1.1.0h --with-libevent-dir=/usr/local/var/homebrew/linked/libevent --with-zlib-dir=/usr/local/Cellar/zlib/1.2.11
#Use homebrew to install openssl1.1 such as
    #brew install openssl@1.1

#Fix for xCode to stop complaining about not finding the string.h file
# https://stackoverflow.com/questions/48839127/qmake-derived-clang-in-osx-10-13-cannot-find-string-h
#INCLUDEPATH += /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/usr/include

ICON = $$PWD/../spectre.icns

    #to build tor use ./configure --with-ssl-dir=/usr/local/Cellar/openssl@1.1/ command in the subfolder of berklydb
    LIBS += -L$$PWD/../tor/src/or -ltor \
    -L$$PWD/../tor/src/common -lor \
    -L$$PWD/../tor/src/common -lor-ctime \
    -L$$PWD/../tor/src/common -lor-crypto \
    -L$$PWD/../tor/src/common -lor-event \
    -L$$PWD/../tor/src/trunnel -lor-trunnel \
    -L$$PWD/../tor/src/common -lcurve25519_donna \
    -L$$PWD/../tor/src/ext/ed25519/donna -led25519_donna \
    -L$$PWD/../tor/src/ext/ed25519/ref10 -led25519_ref10 \
    -L$$PWD/../tor/src/ext/keccak-tiny -lkeccak-tiny \

    INCLUDEPATH += $$PWD/../tor/src/or $$PWD/../tor/src/common $$PWD/../tor/src/trunnel $$PWD/../tor/src/ext/ed25519/donna $$PWD/../tor/src/ext/ed25519/ref10 $$PWD/../tor/src/ext/keccak-tiny
    DEPENDPATH += $$PWD/../tor/src/or $$PWD/../tor/src/common $$PWD/../tor/src/trunnel $$PWD/../tor/src/ext/ed25519/donna $$PWD/../tor/src/ext/ed25519/ref10 $$PWD/../tor/src/ext/keccak-tiny

    PRE_TARGETDEPS += $$PWD/../tor/src/or/libtor.a \
    $$PWD/../tor/src/common/libor.a \
    $$PWD/../tor/src/common/libor-ctime.a \
    $$PWD/../tor/src/common/libor-crypto.a \
    $$PWD/../tor/src/common/libor-event.a \
    $$PWD/../tor/src/trunnel/libor-trunnel.a \
    $$PWD/../tor/src/common/libcurve25519_donna.a \
    $$PWD/../tor/src/ext/ed25519/donna/libed25519_donna.a \
    $$PWD/../tor/src/ext/ed25519/ref10/libed25519_ref10.a \
    $$PWD/../tor/src/ext/keccak-tiny/libkeccak-tiny.a \

#    #brew install zlib
    _ZLIB_PATH = /usr/local/opt/zlib
    INCLUDEPATH += "$${_ZLIB_PATH}/include/"
    LIBS += -L$${_ZLIB_PATH}/lib
    #Shblis-MacBook-Pro:src Shbli$ find /usr/local/Cellar/libevent/2.1.8/lib/ -name *a
    LIBS += -lz

#to build berkly db use ./configure command in the subfolder of berklydb
    LIBS += -L$$PWD/../db4.8/build_unix -ldb_cxx \
    -L$$PWD/../db4.8/build_unix -ldb

    INCLUDEPATH += $$PWD/../db4.8/build_unix
    DEPENDPATH += $$PWD/../db4.8/build_unix

    PRE_TARGETDEPS += $$PWD/../db4.8/build_unix/libdb_cxx.a \
    $$PWD/../db4.8/build_unix/libdb.a


#to build leveldb use ./configure command in the subfolder of leveldb
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
# Mac: compile for maximum compatibility (10.0, 32-bit)
QMAKE_CXXFLAGS += -std=c++14 -mmacosx-version-min=10.10 -isysroot

    # https://www.reddit.com/r/cpp/comments/334s4r/how_to_enable_c14_in_qt_creator_on_a_mac/
    # Turns out, there's a glitch in the Mac version where the standard library isn't correctly included when using the C++14 config flag. Adding this additional line to the .pro file fixes the problem:
    QMAKE_CXXFLAGS += -stdlib=libc++

    #add in core foundation framework
    QMAKE_LFLAGS += -F /System/Library/Frameworks/CoreFoundation.framework/
    LIBS += -framework CoreFoundation

    QMAKE_LFLAGS += -F /System/Library/Frameworks/AppKit.framework/
    LIBS += -framework AppKit

    QMAKE_LFLAGS += -F /System/Library/Frameworks/ApplicationServices.framework/
    LIBS += -framework ApplicationServices


    #brew install boost
    _BOOST_PATH = /usr/local/Cellar/boost/1.67.0_1
    INCLUDEPATH += "$${_BOOST_PATH}/include/"
    LIBS += -L$${_BOOST_PATH}/lib
    LIBS += -lboost_system-mt -lboost_chrono-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt -lboost_unit_test_framework-mt # using dynamic lib (not sure if you need that "-mt" at the end or not)
    DEFINES += BOOST_ASIO_ENABLE_OLD_SERVICES BOOST_SPIRIT_THREADSAFE BOOST_THREAD_USE_LIB BOOST_THREAD_POSIX BOOST_HAS_THREADS
    #LIBS += $${_BOOST_PATH}/lib/libboost_chrono-mt.a # using static lib

    #brew install openssl@1.1
    _OPENSSL_PATH = /usr/local/Cellar/openssl@1.1/1.1.0h
    INCLUDEPATH += "$${_OPENSSL_PATH}/include/"
    LIBS += -L$${_OPENSSL_PATH}/lib
    LIBS += -lssl -lcrypto # using dynamic lib (not sure if you need that "-mt" at the end or not)

    #libevent-2.1.6.dylib
    #brew install libevent
    _LIBEVENT_PATH = /usr/local/Cellar/libevent/2.1.8
    INCLUDEPATH += "$${_LIBEVENT_PATH}/include/"
    LIBS += -L$${_LIBEVENT_PATH}/lib
    #Shblis-MacBook-Pro:src Shbli$ find /usr/local/Cellar/libevent/2.1.8/lib/ -name *a
    LIBS += -levent_extra -levent -levent_core -levent_openssl -levent_pthreads
