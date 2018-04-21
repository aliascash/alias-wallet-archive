#find ./tor -type d
#Note, to compile the tor pojrect use the following command
#./configure --enable-static-tor --with-openssl-dir=/usr/local/Cellar/openssl@1.1/1.1.0h --with-libevent-dir=/usr/local/var/homebrew/linked/libevent --with-zlib-dir=/usr/local/Cellar/zlib/1.2.11

macx {
    LIBS += -L$$PWD/src/or -ltor \
    -L$$PWD/src/common -lor \
    -L$$PWD/src/common -lor-ctime \
    -L$$PWD/src/common -lor-crypto \
    -L$$PWD/src/common -lor-event \
    -L$$PWD/src/trunnel -lor-trunnel \
    -L$$PWD/src/common -lcurve25519_donna \
    -L$$PWD/src/ext/ed25519/donna -led25519_donna \
    -L$$PWD/src/ext/ed25519/ref10 -led25519_ref10 \
    -L$$PWD/src/ext/keccak-tiny -lkeccak-tiny \

    INCLUDEPATH += $$PWD/src/or $$PWD/src/common $$PWD/src/trunnel $$PWD/src/ext/ed25519/donna $$PWD/src/ext/ed25519/ref10 $$PWD/src/ext/keccak-tiny
    DEPENDPATH += $$PWD/src/or $$PWD/src/common $$PWD/src/trunnel $$PWD/src/ext/ed25519/donna $$PWD/src/ext/ed25519/ref10 $$PWD/src/ext/keccak-tiny

    PRE_TARGETDEPS += $$PWD/src/or/libtor.a \
    $$PWD/src/common/libor.a \
    $$PWD/src/common/libor-ctime.a \
    $$PWD/src/common/libor-crypto.a \
    $$PWD/src/common/libor-event.a \
    $$PWD/src/trunnel/libor-trunnel.a \
    $$PWD/src/common/libcurve25519_donna.a \
    $$PWD/src/ext/ed25519/donna/libed25519_donna.a \
    $$PWD/src/ext/ed25519/ref10/libed25519_ref10.a \
    $$PWD/src/ext/keccak-tiny/libkeccak-tiny.a \

#    #brew install zlib
    _ZLIB_PATH = /usr/local/opt/zlib
    INCLUDEPATH += "$${_ZLIB_PATH}/include/"
    LIBS += -L$${_ZLIB_PATH}/lib
    #Shblis-MacBook-Pro:src Shbli$ find /usr/local/Cellar/libevent/2.1.8/lib/ -name *a
    LIBS += -lz
}
