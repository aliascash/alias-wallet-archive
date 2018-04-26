TEMPLATE = app
TARGET = spectre
CONFIG += c++14
#static for VS
#CONFIG += static
#QMAKE_LFLAGS += -static
#QMAKE_CXXFLAGS += /MT
#QMAKE_CFLAGS += -O2 -MT
#QMAKE_CXXFLAGS += -O2 -MT



CONFIG(release, debug|release) {
    message( "release" )
    LIBSPATH = $$PWD/../packages64bit/lib
    DESTDIR = $$PWD/bin
}
CONFIG(debug, debug|release) {
    message( "debug" )
    LIBSPATH = $$PWD/../packages64bit/debug/lib
    DESTDIR = $$PWD/bin/debug
}


#Command to install dependencies (Static)
#vcpkg.exe install boost:x64-windows-static berkeleydb:x64-windows-static leveldb:x64-windows-static libevent:x64-windows-static lua:x64-windows-static openssl:x64-windows-static zlib:x64-windows-static
#Command to install dependencies (Dynamic)
#vcpkg.exe install boost:x64-windows berkeleydb:x64-windows leveldb:x64-windows libevent:x64-windows lua:x64-windows openssl:x64-windows zlib:x64-windows
DEFINES += BOOST_ASIO_ENABLE_OLD_SERVICES BOOST_SPIRIT_THREADSAFE BOOST_THREAD_USE_LIB
#DEFINES += BOOST_DISABLE_CURRENT_FUNCTION



INCLUDEPATH += $$PWD/../packages64bit/include
DEPENDPATH += $$PWD/../packages64bit/include

LIBS += -L$${LIBSPATH}/ -llua
PRE_TARGETDEPS += $${LIBSPATH}/lua.lib
LIBS += -L$${LIBSPATH} -lboost_chrono-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_chrono-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_filesystem-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_filesystem-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_program_options-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_program_options-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_system-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_system-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_thread-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_thread-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lzlib
PRE_TARGETDEPS += $${LIBSPATH}/zlib.lib
LIBS += -L$${LIBSPATH}/ -llibcryptoMD
PRE_TARGETDEPS += $${LIBSPATH}/libcryptoMD.lib
LIBS += -L$${LIBSPATH}/ -llibsslMD
PRE_TARGETDEPS += $${LIBSPATH}/libsslMD.lib
LIBS += -L$${LIBSPATH}/ -levent
PRE_TARGETDEPS += $${LIBSPATH}/event.lib
LIBS += -L$${LIBSPATH}/ -levent_core
PRE_TARGETDEPS += $${LIBSPATH}/event_core.lib
LIBS += -L$${LIBSPATH}/ -levent_extra
PRE_TARGETDEPS += $${LIBSPATH}/event_extra.lib
LIBS += -L$${LIBSPATH}/ -lWS2_32
PRE_TARGETDEPS += $${LIBSPATH}/WS2_32.lib
LIBS += -L$${LIBSPATH}/ -lAdvAPI32
PRE_TARGETDEPS += $${LIBSPATH}/AdvAPI32.lib
LIBS += -L$${LIBSPATH}/ -lshell32
PRE_TARGETDEPS += $${LIBSPATH}/shell32.lib
LIBS += -L$${LIBSPATH}/ -lUser32
PRE_TARGETDEPS += $${LIBSPATH}/User32.lib
LIBS += -L$${LIBSPATH}/ -lCrypt32
PRE_TARGETDEPS += $${LIBSPATH}/Crypt32.lib
#ShLwApi
LIBS += -L$${LIBSPATH}/ -lShLwApi
PRE_TARGETDEPS += $${LIBSPATH}/ShLwApi.lib

#LevelDB from VCPKG
LIBS += -L$${LIBSPATH}/ -llibleveldb
PRE_TARGETDEPS += $${LIBSPATH}/libleveldb.lib
#BerklyDB4.8 from our project repo (VCPKG version is not supported)
LIBS += -L$${LIBSPATH}/ -llibdb48
PRE_TARGETDEPS += $${LIBSPATH}/libdb48.lib
#libdb_stl48.lib (We need both libdb and libdbstl (Standered library)
LIBS += -L$${LIBSPATH}/ -llibdb_stl48
PRE_TARGETDEPS += $${LIBSPATH}/libdb_stl48.lib

LIBS += -L$$PWD/../tor/ -ltorlib
PRE_TARGETDEPS += $$PWD/../tor/torlib.lib
LIBS += -L$$PWD/../leveldb/ -lleveldblib
PRE_TARGETDEPS += $$PWD/../leveldb/leveldblib.lib
#LIBS += -L$$PWD/../db4.8/ -lberkeleydblib
#PRE_TARGETDEPS += $$PWD/../db4.8/berkeleydblib.lib

QT += testlib webenginewidgets webchannel

HEADERS += \
    $$PWD/json/json_spirit.h \
    $$PWD/json/json_spirit_error_position.h \
    $$PWD/json/json_spirit_reader.h \
    $$PWD/json/json_spirit_reader_template.h \
    $$PWD/json/json_spirit_stream_reader.h \
    $$PWD/json/json_spirit_utils.h \
    $$PWD/json/json_spirit_value.h \
    $$PWD/json/json_spirit_writer.h \
    $$PWD/json/json_spirit_writer_template.h \
    $$PWD/lz4/lz4.h \
    $$PWD/qt/test/uritests.h \
    $$PWD/qt/aboutdialog.h \
    $$PWD/qt/addresstablemodel.h \
    $$PWD/qt/askpassphrasedialog.h \
    $$PWD/qt/bitcoinaddressvalidator.h \
    $$PWD/qt/bitcoinamountfield.h \
    $$PWD/qt/bitcoinunits.h \
    $$PWD/qt/bridgetranslations.h \
    $$PWD/qt/clientmodel.h \
    $$PWD/qt/coincontroldialog.h \
    $$PWD/qt/coincontroltreewidget.h \
    $$PWD/qt/csvmodelwriter.h \
    $$PWD/qt/editaddressdialog.h \
    $$PWD/qt/guiconstants.h \
    $$PWD/qt/guiutil.h \
    $$PWD/qt/messagemodel.h \
    $$PWD/qt/monitoreddatamapper.h \
    $$PWD/qt/notificator.h \
    $$PWD/qt/optionsmodel.h \
    $$PWD/qt/paymentserver.h \
    $$PWD/qt/peertablemodel.h \
    $$PWD/qt/qvalidatedlineedit.h \
    $$PWD/qt/qvaluecombobox.h \
    $$PWD/qt/rpcconsole.h \
    $$PWD/qt/scicon.h \
    $$PWD/qt/spectrebridge.h \
    $$PWD/qt/spectregui.h \
    $$PWD/qt/trafficgraphwidget.h \
    $$PWD/qt/transactiondesc.h \
    $$PWD/qt/transactionrecord.h \
    $$PWD/qt/transactiontablemodel.h \
    $$PWD/qt/walletmodel.h \
    $$PWD/wordlists/chinese_simplified.h \
    $$PWD/wordlists/chinese_traditional.h \
    $$PWD/wordlists/english.h \
    $$PWD/wordlists/french.h \
    $$PWD/wordlists/japanese.h \
    $$PWD/wordlists/spanish.h \
    $$PWD/xxhash/xxhash.h \
    $$PWD/alert.h \
    $$PWD/allocators.h \
    $$PWD/anonymize.h \
    $$PWD/base58.h \
    $$PWD/bignum.h \
    $$PWD/bloom.h \
    $$PWD/chainparams.h \
    $$PWD/chainparamsseeds.h \
    $$PWD/checkpoints.h \
    $$PWD/clientversion.h \
    $$PWD/coincontrol.h \
    $$PWD/compat.h \
    $$PWD/core.h \
    $$PWD/crypter.h \
    $$PWD/db.h \
    $$PWD/eckey.h \
    $$PWD/extkey.h \
    $$PWD/hash.h \
    $$PWD/init.h \
    $$PWD/kernel.h \
    $$PWD/key.h \
    $$PWD/keystore.h \
    $$PWD/main.h \
    $$PWD/miner.h \
    $$PWD/mruset.h \
    $$PWD/net.h \
    $$PWD/netbase.h \
    $$PWD/pbkdf2.h \
    $$PWD/protocol.h \
    $$PWD/ringsig.h \
    $$PWD/rpcclient.h \
    $$PWD/rpcprotocol.h \
    $$PWD/rpcserver.h \
    $$PWD/script.h \
    $$PWD/scrypt.h \
    $$PWD/serialize.h \
    $$PWD/smessage.h \
    $$PWD/state.h \
    $$PWD/stealth.h \
    $$PWD/strlcpy.h \
    $$PWD/sync.h \
    $$PWD/threadsafety.h \
    $$PWD/tinyformat.h \
    $$PWD/txdb-leveldb.h \
    $$PWD/txdb.h \
    $$PWD/txmempool.h \
    $$PWD/types.h \
    $$PWD/ui_interface.h \
    $$PWD/uint256.h \
    $$PWD/util.h \
    $$PWD/version.h \
    $$PWD/wallet.h \
    $$PWD/walletdb.h \
    $$PWD/addrman.h \
    unistd.h

SOURCES += \
    $$PWD/json/json_spirit_reader.cpp \
    $$PWD/json/json_spirit_value.cpp \
    $$PWD/json/json_spirit_writer.cpp \
#    $$PWD/qt/test/test_main.cpp \
    $$PWD/qt/test/uritests.cpp \
    $$PWD/qt/aboutdialog.cpp \
    $$PWD/qt/addresstablemodel.cpp \
    $$PWD/qt/askpassphrasedialog.cpp \
    $$PWD/qt/bitcoinaddressvalidator.cpp \
    $$PWD/qt/bitcoinamountfield.cpp \
    $$PWD/qt/bitcoinstrings.cpp \
    $$PWD/qt/bitcoinunits.cpp \
    $$PWD/qt/clientmodel.cpp \
    $$PWD/qt/coincontroldialog.cpp \
    $$PWD/qt/coincontroltreewidget.cpp \
    $$PWD/qt/csvmodelwriter.cpp \
    $$PWD/qt/editaddressdialog.cpp \
    $$PWD/qt/guiutil.cpp \
    $$PWD/qt/messagemodel.cpp \
    $$PWD/qt/monitoreddatamapper.cpp \
    $$PWD/qt/notificator.cpp \
    $$PWD/qt/optionsmodel.cpp \
    $$PWD/qt/paymentserver.cpp \
    $$PWD/qt/peertablemodel.cpp \
    $$PWD/qt/qvalidatedlineedit.cpp \
    $$PWD/qt/qvaluecombobox.cpp \
    $$PWD/qt/rpcconsole.cpp \
    $$PWD/qt/scicon.cpp \
    $$PWD/qt/spectre.cpp \
    $$PWD/qt/spectrebridge.cpp \
    $$PWD/qt/spectregui.cpp \
    $$PWD/qt/trafficgraphwidget.cpp \
    $$PWD/qt/transactiondesc.cpp \
    $$PWD/qt/transactionrecord.cpp \
    $$PWD/qt/transactiontablemodel.cpp \
    $$PWD/qt/walletmodel.cpp \
#    $$PWD/test/other/DoS_tests.cpp \
#    $$PWD/test/other/miner_tests.cpp \
#    $$PWD/test/other/transaction_tests.cpp \
#    $$PWD/test/accounting_tests.cpp \
#    $$PWD/test/allocator_tests.cpp \
#    $$PWD/test/base32_tests.cpp \
#    $$PWD/test/base58_tests.cpp \
#    $$PWD/test/base64_tests.cpp \
#    $$PWD/test/basic_tests.cpp \
#    $$PWD/test/bignum_tests.cpp \
#    $$PWD/test/bip32_tests.cpp \
#    $$PWD/test/Checkpoints_tests.cpp \
#    $$PWD/test/extkey_tests.cpp \
#    $$PWD/test/getarg_tests.cpp \
#    $$PWD/test/hash_tests.cpp \
#    $$PWD/test/hmac_tests.cpp \
#    $$PWD/test/key_tests.cpp \
#    $$PWD/test/mnemonic_tests.cpp \
#    $$PWD/test/mruset_tests.cpp \
#    $$PWD/test/multisig_tests.cpp \
#    $$PWD/test/netbase_tests.cpp \
#    $$PWD/test/ringsig_tests.cpp \
#    $$PWD/test/rpc_tests.cpp \
#    $$PWD/test/script_P2SH_tests.cpp \
#    $$PWD/test/script_tests.cpp \
#    $$PWD/test/sigopcount_tests.cpp \
#    $$PWD/test/smsg_tests.cpp \
#    $$PWD/test/stealth_tests.cpp \
#    $$PWD/test/test_shadow.cpp \
#    $$PWD/test/uint160_tests.cpp \
#    $$PWD/test/uint256_tests.cpp \
#    $$PWD/test/util_tests.cpp \
#    $$PWD/test/wallet_tests.cpp \
    $$PWD/alert.cpp \
    $$PWD/anonymize.cpp \
    $$PWD/bloom.cpp \
    $$PWD/chainparams.cpp \
    $$PWD/checkpoints.cpp \
    $$PWD/core.cpp \
    $$PWD/crypter.cpp \
    $$PWD/db.cpp \
    $$PWD/eckey.cpp \
    $$PWD/extkey.cpp \
    $$PWD/hash.cpp \
    $$PWD/init.cpp \
    $$PWD/kernel.cpp \
    $$PWD/key.cpp \
    $$PWD/keystore.cpp \
    $$PWD/main.cpp \
    $$PWD/miner.cpp \
    $$PWD/net.cpp \
    $$PWD/netbase.cpp \
    $$PWD/noui.cpp \
    $$PWD/pbkdf2.cpp \
    $$PWD/protocol.cpp \
    $$PWD/ringsig.cpp \
    $$PWD/rpcblockchain.cpp \
    $$PWD/rpcclient.cpp \
    $$PWD/rpcdump.cpp \
    $$PWD/rpcextkey.cpp \
    $$PWD/rpcmining.cpp \
    $$PWD/rpcmnemonic.cpp \
    $$PWD/rpcnet.cpp \
    $$PWD/rpcprotocol.cpp \
    $$PWD/rpcrawtransaction.cpp \
    $$PWD/rpcserver.cpp \
    $$PWD/rpcsmessage.cpp \
    $$PWD/rpcwallet.cpp \
    $$PWD/script.cpp \
    $$PWD/scrypt.cpp \
    $$PWD/smessage.cpp \
#    $$PWD/spectrecoind.cpp \
    $$PWD/state.cpp \
    $$PWD/stealth.cpp \
    $$PWD/sync.cpp \
    $$PWD/txdb-leveldb.cpp \
    $$PWD/txmempool.cpp \
    $$PWD/util.cpp \
    $$PWD/version.cpp \
    $$PWD/wallet.cpp \
    $$PWD/walletdb.cpp \
    $$PWD/lz4/lz4.c \
    $$PWD/xxhash/xxhash.c \
    $$PWD/addrman.cpp \


QMAKE_CFLAGS_WARN_ON -= -W3
QMAKE_CFLAGS_WARN_ON += -W2

QMAKE_CXXFLAGS_WARN_ON -= -W3
QMAKE_CXXFLAGS_WARN_ON += -W2

#levelDB additional headers
INCLUDEPATH += $$PWD/../leveldb/helpers
#find . -type d
INCLUDEPATH += $$PWD $$PWD/obj $$PWD/test $$PWD/test/other $$PWD/test/data $$PWD/wordlists $$PWD/qt $$PWD/qt/res $$PWD/qt/res/css $$PWD/qt/res/css/fonts $$PWD/qt/res/images $$PWD/qt/res/images/avatars $$PWD/qt/res/icons $$PWD/qt/res/assets $$PWD/qt/res/assets/css $$PWD/qt/res/assets/plugins $$PWD/qt/res/assets/plugins/md5 $$PWD/qt/res/assets/plugins/identicon $$PWD/qt/res/assets/plugins/boostrapv3 $$PWD/qt/res/assets/plugins/boostrapv3/css $$PWD/qt/res/assets/plugins/boostrapv3/js $$PWD/qt/res/assets/plugins/boostrapv3/fonts $$PWD/qt/res/assets/plugins/framework $$PWD/qt/res/assets/plugins/markdown $$PWD/qt/res/assets/plugins/shajs $$PWD/qt/res/assets/plugins/pnglib $$PWD/qt/res/assets/plugins/iscroll $$PWD/qt/res/assets/plugins/jquery $$PWD/qt/res/assets/plugins/classie $$PWD/qt/res/assets/plugins/pace $$PWD/qt/res/assets/plugins/contextMenu $$PWD/qt/res/assets/plugins/jquery-scrollbar $$PWD/qt/res/assets/plugins/jdenticon $$PWD/qt/res/assets/plugins/qrcode $$PWD/qt/res/assets/plugins/emojione $$PWD/qt/res/assets/plugins/emojione/assets $$PWD/qt/res/assets/plugins/emojione/assets/svg $$PWD/qt/res/assets/plugins/emojione/assets/css $$PWD/qt/res/assets/plugins/jquery-transit $$PWD/qt/res/assets/plugins/footable $$PWD/qt/res/assets/plugins/jquery-ui $$PWD/qt/res/assets/plugins/jquery-ui/images $$PWD/qt/res/assets/js $$PWD/qt/res/assets/js/pages $$PWD/qt/res/assets/img $$PWD/qt/res/assets/img/progress $$PWD/qt/res/assets/img/avatars $$PWD/qt/res/assets/icons $$PWD/qt/res/assets/fonts $$PWD/qt/res/assets/fonts/Framework-icon $$PWD/qt/res/assets/fonts/FontAwesome $$PWD/qt/res/assets/fonts/Montserrat $$PWD/qt/res/assets/fonts/Footable $$PWD/qt/res/src $$PWD/qt/locale $$PWD/qt/forms $$PWD/qt/test $$PWD/lz4 $$PWD/json $$PWD/xxhash $$PWD/obj-test

    QMAKE_CXXFLAGS += -O2 -D_FORTIFY_SOURCE=1

macx {

HEADERS +=              $$PWD/qt/macdockiconhandler.h \
                        $$PWD/qt/macnotificationhandler.h \

OBJECTIVE_SOURCES +=    $$PWD/qt/macdockiconhandler.mm \
                        $$PWD/qt/macnotificationhandler.mm \

    QMAKE_CXXFLAGS += -pthread -fPIC -fstack-protector -O2 \
                  -D_FORTIFY_SOURCE=1 \
                  -Wall -Wextra -Wno-ignored-qualifiers -Woverloaded-virtual \
                  -Wformat -Wformat-security -Wno-unused-parameter

    DEFINES += MAC_OSX
    # Mac: compile for maximum compatibility (10.0, 32-bit)
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.10 -isysroot

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
    LIBS += -lboost_system-mt -lboost_chrono-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_thread-mt # using dynamic lib (not sure if you need that "-mt" at the end or not)
    DEFINES += BOOST_ASIO_ENABLE_OLD_SERVICES BOOST_SPIRIT_THREADSAFE BOOST_THREAD_USE_LIB
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
}

FORMS += \
    $$PWD/qt/forms/aboutdialog.ui \
    $$PWD/qt/forms/askpassphrasedialog.ui \
    $$PWD/qt/forms/coincontroldialog.ui \
    $$PWD/qt/forms/editaddressdialog.ui \
    $$PWD/qt/forms/rpcconsole.ui \
    $$PWD/qt/forms/transactiondescdialog.ui

RESOURCES += \
    ../spectre.qrc

