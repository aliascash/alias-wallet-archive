TEMPLATE = app
TARGET = spectre_tests
CONFIG += c++14

DEFINES += DEBUGGER_CONNECTED
#DEFINES += TEST_TOR
DEFINES += SPECTRE_QT_TEST
DEFINES += CURRENT_PATH=\\\"$$PWD\\\"
DEFINES += BOOST_TEST_DYN_LINK

QT += testlib webenginewidgets webchannel

RC_FILE = src.rc

CONFIG(release, debug|release) {
    message( "release" )
    DESTDIR = $$PWD/bin
}
CONFIG(debug, debug|release) {
    message( "debug" )
    DESTDIR = $$PWD/bin/debug
}

macx {
    message(Mac build)
    include(osx.pri)
} win32 {
    message(Win build)
    include(win.pri)
}

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

SOURCES += \
    $$PWD/json/json_spirit_reader.cpp \
    $$PWD/json/json_spirit_value.cpp \
    $$PWD/json/json_spirit_writer.cpp \
    $$PWD/qt/test/test_main.cpp \
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
    $$PWD/test/other/DoS_tests.cpp \
#    $$PWD/test/other/miner_tests.cpp \
    $$PWD/test/other/transaction_tests.cpp \
    $$PWD/test/accounting_tests.cpp \
    $$PWD/test/allocator_tests.cpp \
    $$PWD/test/base32_tests.cpp \
    $$PWD/test/base58_tests.cpp \
    $$PWD/test/base64_tests.cpp \
    $$PWD/test/basic_tests.cpp \
    $$PWD/test/bignum_tests.cpp \
    $$PWD/test/bip32_tests.cpp \
    $$PWD/test/Checkpoints_tests.cpp \
    $$PWD/test/extkey_tests.cpp \
    $$PWD/test/getarg_tests.cpp \
    $$PWD/test/hash_tests.cpp \
    $$PWD/test/hmac_tests.cpp \
    $$PWD/test/key_tests.cpp \
    $$PWD/test/mnemonic_tests.cpp \
    $$PWD/test/mruset_tests.cpp \
    $$PWD/test/multisig_tests.cpp \
    $$PWD/test/netbase_tests.cpp \
    $$PWD/test/ringsig_tests.cpp \
    $$PWD/test/rpc_tests.cpp \
    $$PWD/test/script_P2SH_tests.cpp \
    $$PWD/test/script_tests.cpp \
    $$PWD/test/sigopcount_tests.cpp \
    $$PWD/test/smsg_tests.cpp \
    $$PWD/test/stealth_tests.cpp \
#    $$PWD/test/test_shadow.cpp \
    $$PWD/test/uint160_tests.cpp \
    $$PWD/test/uint256_tests.cpp \
    $$PWD/test/util_tests.cpp \
    $$PWD/test/wallet_tests.cpp \
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


#levelDB additional headers
INCLUDEPATH += $$PWD/../leveldb/helpers
#find . -type d
INCLUDEPATH += $$PWD $$PWD/obj $$PWD/test $$PWD/test/other $$PWD/test/data $$PWD/wordlists $$PWD/qt $$PWD/qt/res $$PWD/qt/res/css $$PWD/qt/res/css/fonts $$PWD/qt/res/images $$PWD/qt/res/images/avatars $$PWD/qt/res/icons $$PWD/qt/res/assets $$PWD/qt/res/assets/css $$PWD/qt/res/assets/plugins $$PWD/qt/res/assets/plugins/md5 $$PWD/qt/res/assets/plugins/identicon $$PWD/qt/res/assets/plugins/boostrapv3 $$PWD/qt/res/assets/plugins/boostrapv3/css $$PWD/qt/res/assets/plugins/boostrapv3/js $$PWD/qt/res/assets/plugins/boostrapv3/fonts $$PWD/qt/res/assets/plugins/framework $$PWD/qt/res/assets/plugins/markdown $$PWD/qt/res/assets/plugins/shajs $$PWD/qt/res/assets/plugins/pnglib $$PWD/qt/res/assets/plugins/iscroll $$PWD/qt/res/assets/plugins/jquery $$PWD/qt/res/assets/plugins/classie $$PWD/qt/res/assets/plugins/pace $$PWD/qt/res/assets/plugins/contextMenu $$PWD/qt/res/assets/plugins/jquery-scrollbar $$PWD/qt/res/assets/plugins/jdenticon $$PWD/qt/res/assets/plugins/qrcode $$PWD/qt/res/assets/plugins/emojione $$PWD/qt/res/assets/plugins/emojione/assets $$PWD/qt/res/assets/plugins/emojione/assets/svg $$PWD/qt/res/assets/plugins/emojione/assets/css $$PWD/qt/res/assets/plugins/jquery-transit $$PWD/qt/res/assets/plugins/footable $$PWD/qt/res/assets/plugins/jquery-ui $$PWD/qt/res/assets/plugins/jquery-ui/images $$PWD/qt/res/assets/js $$PWD/qt/res/assets/js/pages $$PWD/qt/res/assets/img $$PWD/qt/res/assets/img/progress $$PWD/qt/res/assets/img/avatars $$PWD/qt/res/assets/icons $$PWD/qt/res/assets/fonts $$PWD/qt/res/assets/fonts/Framework-icon $$PWD/qt/res/assets/fonts/FontAwesome $$PWD/qt/res/assets/fonts/Montserrat $$PWD/qt/res/assets/fonts/Footable $$PWD/qt/res/src $$PWD/qt/locale $$PWD/qt/forms $$PWD/qt/test $$PWD/lz4 $$PWD/json $$PWD/xxhash $$PWD/obj-test


FORMS += \
    $$PWD/qt/forms/aboutdialog.ui \
    $$PWD/qt/forms/askpassphrasedialog.ui \
    $$PWD/qt/forms/coincontroldialog.ui \
    $$PWD/qt/forms/editaddressdialog.ui \
    $$PWD/qt/forms/rpcconsole.ui \
    $$PWD/qt/forms/transactiondescdialog.ui

RESOURCES += \
    ../spectre.qrc
