TEMPLATE = app
TARGET = spectre
VERSION = 1.3.0
INCLUDEPATH += src src/json src/qt src/tor
DEFINES += BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE
CONFIG += no_include_pwd
CONFIG += thread
CONFIG += static

greaterThan(QT_MAJOR_VERSION, 4) {
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
}

# for boost 1.37, add -mt to the boost libraries
# use: qmake BOOST_LIB_SUFFIX=-mt
# for boost thread win32 with _win32 sufix
# use: BOOST_THREAD_LIB_SUFFIX=_win32-...
# or when linking against a specific BerkelyDB version: BDB_LIB_SUFFIX=-4.8

# Dependency library locations can be customized with:
#    BOOST_INCLUDE_PATH, BOOST_LIB_PATH, BDB_INCLUDE_PATH,
#    BDB_LIB_PATH, OPENSSL_INCLUDE_PATH and OPENSSL_LIB_PATH respectively

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
RESOURCES = spectre.qrc

QT += webkit network

build_macosx64 {
    QMAKE_TARGET_BUNDLE_PREFIX = co.spectrecoin
    BOOST_LIB_SUFFIX=-mt
    BOOST_INCLUDE_PATH=/usr/local/Cellar/boost/1.61.0_1/include
    BOOST_LIB_PATH=/usr/local/Cellar/boost/1.61.0_1/lib

    BDB_INCLUDE_PATH=/usr/local/opt/berkeley-db4/include
    BDB_LIB_PATH=/usr/local/Cellar/berkeley-db4/4.8.30/lib

    OPENSSL_INCLUDE_PATH=/usr/local/opt/openssl/include
    OPENSSL_LIB_PATH=/usr/local/opt/openssl/lib

    #MINIUPNPC_INCLUDE_PATH=/usr/local/opt/miniupnpc/include
    #MINIUPNPC_LIB_PATH=/usr/local/Cellar/miniupnpc/1.8.20131007/lib
    MINIUPNPC_INCLUDE_PATH=/usr/local/Cellar/miniupnpc/2.0/include
    MINIUPNPC_LIB_PATH=/usr/local/Cellar/miniupnpc/2.0/lib

    QMAKE_CXXFLAGS += -arch x86_64 -stdlib=libc++
    QMAKE_CFLAGS += -arch x86_64
    QMAKE_LFLAGS += -arch x86_64 -stdlib=libc++
    USE_UPNP=1

}
build_win32 {

    BOOST_LIB_SUFFIX=-mgw49-mt-s-1_55
    BOOST_INCLUDE_PATH=C:\deps\32\boost_1_55_0
    BOOST_LIB_PATH=C:\deps\32\boost_1_55_0\stage\lib

    BDB_INCLUDE_PATH=C:\deps\32\db-4.8.30.NC\build_unix
    BDB_LIB_PATH=C:\deps\32\db-4.8.30.NC\build_unix
	
    OPENSSL_INCLUDE_PATH=C:\deps\32\openssl-1.0.1j\include
    OPENSSL_LIB_PATH=C:\deps\32\openssl-1.0.1j

    MINIUPNPC_INCLUDE_PATH=C:\deps\32\miniupnpc
    MINIUPNPC_LIB_PATH=C:\deps\32\miniupnpc

        #USE_BUILD_INFO = 1
        DEFINES += HAVE_BUILD_INFO

    #USE_UPNP=-
}

# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
    CONFIG += static

    !windows:!macx {
        # Linux: static link
        LIBS += -Wl,-Bstatic
    }
}

# for extra security against potential buffer overflows: enable GCCs Stack Smashing Protection
QMAKE_CXXFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
QMAKE_LFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
# We need to exclude this for Windows cross compile with MinGW 4.2.x, as it will result in a non-working executable!
# This can be enabled for Windows, when we switch to MinGW >= 4.4.x.
# for extra security on Windows: enable ASLR and DEP via GCC linker flags
win32:QMAKE_LFLAGS *= -Wl,--dynamicbase -Wl,--nxcompat -static
#win32:QMAKE_LFLAGS *= -Wl,--large-address-aware  # for 32-bit only
win32:QMAKE_LFLAGS *= -static-libgcc -static-libstdc++

contains(SPECTRE_NEED_QT_PLUGINS, 1) {
    DEFINES += SPECTRE_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs qtaccessiblewidgets
}

INCLUDEPATH += src/leveldb/include src/leveldb/helpers
LIBS += $$PWD/src/leveldb/libleveldb.a $$PWD/src/leveldb/libmemenv.a
SOURCES += src/txdb-leveldb.cpp \
    src/qt/addresstablemodel.cpp

win32 {
    # make an educated guess about what the ranlib command is called
    isEmpty(QMAKE_RANLIB) {
        QMAKE_RANLIB = $$replace(QMAKE_STRIP, strip, ranlib)
    }
    LIBS += -lshlwapi
    genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX TARGET_OS=OS_WINDOWS_CROSSCOMPILE $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libleveldb.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libmemenv.a
} else:macx {
    # we use QMAKE_CXXFLAGS_RELEASE even without RELEASE=1 because we use RELEASE to indicate linking preferences not -O preferences
    genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX AR=$${QMAKE_HOST}-ar TARGET_OS=Darwin $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a
} else {
    genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a
}
genleveldb.target = $$PWD/src/leveldb/libleveldb.a
genleveldb.depends = FORCE
PRE_TARGETDEPS += $$PWD/src/leveldb/libleveldb.a
QMAKE_EXTRA_TARGETS += genleveldb
# Gross ugly hack that depends on qmake internals, unfortunately there is no other way to do it.
QMAKE_CLEAN += $$PWD/src/leveldb/libleveldb.a; cd $$PWD/src/leveldb ; $(MAKE) clean

# regenerate src/build.h
!windows|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh $$OUT_PWD/build/build.h
    genbuild.target = $$OUT_PWD/build/build.h
    PRE_TARGETDEPS += $$OUT_PWD/build/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

contains(USE_O3, 1) {
    message(Building O3 optimization flag)
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS += -O3
    QMAKE_CFLAGS += -O3
}

*-g++-32 {
    message("32 platform, adding -msse2 flag")

    QMAKE_CXXFLAGS += -msse2
    QMAKE_CFLAGS += -msse2
}

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wno-ignored-qualifiers -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector

# Input
DEPENDPATH += src src/json src/qt
HEADERS += \
    src/alert.h \
    src/allocators.h \
    src/wallet.h \
    src/keystore.h \
    src/version.h \
    src/netbase.h \
    src/clientversion.h \
    src/threadsafety.h \
    src/protocol.h \
    src/ui_interface.h \
    src/crypter.h \
    src/addrman.h \
    src/base58.h \
    src/bignum.h \
    src/chainparams.h \
    src/checkpoints.h \
    src/compat.h \
    src/coincontrol.h \
    src/sync.h \
    src/util.h \
    src/hash.h \
    src/uint256.h \
    src/kernel.h \
    src/scrypt.h \
    src/pbkdf2.h \
    src/serialize.h \
    src/strlcpy.h \
    src/smessage.h \
    src/main.h \
    src/miner.h \
    src/net.h \
    src/key.h \
    src/extkey.h \
    src/eckey.h \
    src/db.h \
    src/txdb.h \
    src/walletdb.h \
    src/script.h \
    src/stealth.h \
    src/ringsig.h  \
    src/core.h  \
    src/txmempool.h  \
    src/state.h \
    src/bloom.h \
    src/init.h \
    src/mruset.h \
    src/rpcprotocol.h \
    src/rpcserver.h \
    src/rpcclient.h \
    src/json/json_spirit_writer_template.h \
    src/json/json_spirit_writer.h \
    src/json/json_spirit_value.h \
    src/json/json_spirit_utils.h \
    src/json/json_spirit_stream_reader.h \
    src/json/json_spirit_reader_template.h \
    src/json/json_spirit_reader.h \
    src/json/json_spirit_error_position.h \
    src/json/json_spirit.h \
    src/qt/transactiontablemodel.h \
    src/qt/addresstablemodel.h \
    src/qt/coincontroldialog.h \
    src/qt/coincontroltreewidget.h \
    src/qt/aboutdialog.h \
    src/qt/editaddressdialog.h \
    src/qt/bitcoinaddressvalidator.h \
    src/qt/clientmodel.h \
    src/qt/guiutil.h \
    src/qt/transactionrecord.h \
    src/qt/guiconstants.h \
    src/qt/optionsmodel.h \
    src/qt/monitoreddatamapper.h \
    src/qt/transactiondesc.h \
    src/qt/bitcoinamountfield.h \
    src/qt/walletmodel.h \
    src/qt/csvmodelwriter.h \
    src/qt/qvalidatedlineedit.h \
    src/qt/bitcoinunits.h \
    src/qt/qvaluecombobox.h \
    src/qt/askpassphrasedialog.h \
    src/qt/notificator.h \
    src/qt/rpcconsole.h \
    src/qt/paymentserver.h \
    src/qt/peertablemodel.h \
    src/qt/scicon.h \
    src/qt/trafficgraphwidget.h \
    src/qt/messagemodel.h \
    src/qt/spectregui.h \
    src/qt/spectrebridge.h \
    src/qt/bridgetranslations.h

SOURCES += \
    src/alert.cpp \
    src/version.cpp \
    src/chainparams.cpp \
    src/sync.cpp \
    src/smessage.cpp \
    src/util.cpp \
    src/hash.cpp \
    src/netbase.cpp \
    src/key.cpp \
    src/extkey.cpp \
    src/eckey.cpp \
    src/script.cpp \
    src/main.cpp \
    src/miner.cpp \
    src/init.cpp \
    src/net.cpp \
    src/checkpoints.cpp \
    src/addrman.cpp \
    src/db.cpp \
    src/walletdb.cpp \
    src/noui.cpp \
    src/kernel.cpp \
    src/scrypt-arm.S \
    src/scrypt-x86.S \
    src/scrypt-x86_64.S \
    src/scrypt.cpp \
    src/pbkdf2.cpp \
    src/stealth.cpp  \
    src/ringsig.cpp  \
    src/core.cpp  \
    src/txmempool.cpp  \
    src/wallet.cpp \
    src/keystore.cpp \
    src/state.cpp \
    src/bloom.cpp \
    src/crypter.cpp \
    src/protocol.cpp \
    src/rpcprotocol.cpp \
    src/rpcserver.cpp \
    src/rpcclient.cpp \
    src/rpcdump.cpp \
    src/rpcnet.cpp \
    src/rpcmining.cpp \
    src/rpcwallet.cpp \
    src/rpcblockchain.cpp \
    src/rpcrawtransaction.cpp \
    src/rpcsmessage.cpp \
    src/rpcextkey.cpp \
    src/rpcmnemonic.cpp \
    src/qt/transactiontablemodel.cpp \
    src/qt/coincontroldialog.cpp \
    src/qt/coincontroltreewidget.cpp \
    src/qt/aboutdialog.cpp \
    src/qt/editaddressdialog.cpp \
    src/qt/bitcoinaddressvalidator.cpp \
    src/qt/clientmodel.cpp \
    src/qt/guiutil.cpp \
    src/qt/transactionrecord.cpp \
    src/qt/optionsmodel.cpp \
    src/qt/monitoreddatamapper.cpp \
    src/qt/transactiondesc.cpp \
    src/qt/bitcoinstrings.cpp \
    src/qt/bitcoinamountfield.cpp \
    src/qt/walletmodel.cpp \
    src/qt/csvmodelwriter.cpp \
    src/qt/qvalidatedlineedit.cpp \
    src/qt/bitcoinunits.cpp \
    src/qt/qvaluecombobox.cpp \
    src/qt/askpassphrasedialog.cpp \
    src/qt/notificator.cpp \
    src/qt/rpcconsole.cpp \
    src/qt/paymentserver.cpp \
    src/qt/peertablemodel.cpp \
    src/qt/scicon.cpp \
    src/qt/trafficgraphwidget.cpp \
    src/qt/messagemodel.cpp \
    src/qt/spectregui.cpp \
    src/qt/spectre.cpp \
    src/qt/spectrebridge.cpp
	
### tor sources
SOURCES += src/tor/anonymize.cpp \
    src/tor/address.c \
    src/tor/addressmap.c \
    src/tor/aes.c \
    src/tor/backtrace.c \
    src/tor/blinding.c \
    src/tor/bridges.c \
    src/tor/buffers.c \
    src/tor/cell_common.c \
    src/tor/cell_establish_intro.c \
    src/tor/cell_introduce1.c \
    src/tor/channel.c \
    src/tor/channeltls.c \
    src/tor/circpathbias.c \
    src/tor/circuitbuild.c \
    src/tor/circuitlist.c \
    src/tor/circuitmux.c \
    src/tor/circuitmux_ewma.c \
    src/tor/circuitstats.c \
    src/tor/circuituse.c \
    src/tor/command.c \
    src/tor/compat_libevent.c \
    src/tor/compat_threads.c \
    src/tor/compat_time.c \
    src/tor/config.c \
    src/tor/confparse.c \
    src/tor/connection.c \
    src/tor/connection_edge.c \
    src/tor/connection_or.c \
    src/tor/container.c \
    src/tor/control.c \
    src/tor/crypto.c \
    src/tor/crypto_curve25519.c \
    src/tor/crypto_ed25519.c \
    src/tor/crypto_format.c \
    src/tor/crypto_pwbox.c \
    src/tor/crypto_s2k.c \
    src/tor/cpuworker.c \
    src/tor/csiphash.c \
    src/tor/curve25519-donna.c \
    src/tor/di_ops.c \
    src/tor/dircollate.c \
    src/tor/directory.c \
    src/tor/dirserv.c \
    src/tor/dirvote.c \
    src/tor/dns.c \
    src/tor/dnsserv.c \
    src/tor/ed25519_cert.c \
    src/tor/ed25519_tor.c \
    src/tor/entrynodes.c \
    src/tor/ext_orport.c \
    src/tor/fe_copy.c \
    src/tor/fe_cmov.c \
    src/tor/fe_isnegative.c \
    src/tor/fe_sq.c \
    src/tor/fe_pow22523.c \
    src/tor/fe_isnonzero.c \
    src/tor/fe_neg.c \
    src/tor/fe_frombytes.c \
    src/tor/fe_invert.c \
    src/tor/fe_sub.c \
    src/tor/fe_add.c \
    src/tor/fe_1.c \
    src/tor/fe_mul.c \
    src/tor/fe_tobytes.c \
    src/tor/fe_0.c \
    src/tor/fe_sq2.c \
    src/tor/fp_pair.c \
    src/tor/ge_scalarmult_base.c \
    src/tor/ge_p3_tobytes.c \
    src/tor/ge_frombytes.c \
    src/tor/ge_double_scalarmult.c \
    src/tor/ge_tobytes.c \
    src/tor/ge_p3_to_cached.c \
    src/tor/ge_p3_to_p2.c \
    src/tor/ge_p3_dbl.c \
    src/tor/ge_p3_0.c \
    src/tor/ge_p1p1_to_p2.c \
    src/tor/ge_p1p1_to_p3.c \
    src/tor/ge_add.c \
    src/tor/ge_p2_0.c \
    src/tor/ge_p2_dbl.c \
    src/tor/ge_madd.c \
    src/tor/ge_msub.c \
    src/tor/ge_sub.c \
    src/tor/ge_precomp_0.c \
    src/tor/geoip.c \
    src/tor/hibernate.c \
    src/tor/hs_cache.c \
    src/tor/hs_circuitmap.c \
    src/tor/hs_common.c \
    src/tor/hs_descriptor.c \
    src/tor/hs_intropoint.c \
    src/tor/hs_service.c \
    src/tor/keyconv.c \
    src/tor/keypair.c \
    src/tor/keypin.c \
    src/tor/keccak-tiny-unrolled.c \
    src/tor/link_handshake.c \
    src/tor/log.c \
    src/tor/tormain.c \
    src/tor/memarea.c \
    src/tor/microdesc.c \
    src/tor/networkstatus.c \
    src/tor/nodelist.c \
    src/tor/ntmain.c \
    src/tor/onion.c \
    src/tor/onion_fast.c \
    src/tor/onion_ntor.c \
    src/tor/onion_tap.c \
    src/tor/open.c \
    src/tor/parsecommon.c \
    src/tor/periodic.c \
    src/tor/policies.c \
    src/tor/procmon.c \
    src/tor/protover.c \
    src/tor/pwbox.c \
    src/tor/reasons.c \
    src/tor/relay.c \
    src/tor/rendcache.c \
    src/tor/rendclient.c \
    src/tor/rendcommon.c \
    src/tor/rendmid.c \
    src/tor/rendservice.c \
    src/tor/rephist.c \
    src/tor/replaycache.c \
    src/tor/router.c \
    src/tor/routerkeys.c \
    src/tor/routerlist.c \
    src/tor/routerparse.c \
    src/tor/routerset.c \
    src/tor/sandbox.c \
    src/tor/sc_reduce.c \
    src/tor/sc_muladd.c \
    src/tor/scheduler.c \
    src/tor/shared_random.c \
    src/tor/shared_random_state.c \
    src/tor/sign.c \
    src/tor/statefile.c \
    src/tor/status.c \
    src/tor/torcert.c \
    src/tor/torcompat.c \
    src/tor/tor_main.c \
    src/tor/torgzip.c \
    src/tor/tortls.c \
    src/tor/torutil.c \
    src/tor/transports.c \
    src/tor/trunnel.c \
    src/tor/util_bug.c \
    src/tor/util_format.c \
    src/tor/util_process.c \
    src/tor/workqueue.c \

win32 {
    SOURCES += src/tor/compat_winthreads.c
} else {
    SOURCES += src/tor/compat_pthreads.c \
        src/tor/readpassphrase.c
}

FORMS += \
    src/qt/forms/coincontroldialog.ui \
    src/qt/forms/aboutdialog.ui \
    src/qt/forms/editaddressdialog.ui \
    src/qt/forms/transactiondescdialog.ui \
    src/qt/forms/askpassphrasedialog.ui \
    src/qt/forms/rpcconsole.ui


CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to spectre.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/umbra*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += \
    .travis.yml doc/*.rst doc/*.txt doc/README README.md res/bitcoin-qt.rc contrib/macdeploy/createdmg

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_LIB_SUFFIX) {
    macx:BOOST_LIB_SUFFIX = -mt
    windows:BOOST_LIB_SUFFIX = -mt
}

isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

isEmpty(BDB_LIB_PATH) {
    macx:BDB_LIB_PATH = /opt/local/lib/db48
}

isEmpty(BDB_LIB_SUFFIX) {
    macx:BDB_LIB_SUFFIX = -4.8
}

isEmpty(BDB_INCLUDE_PATH) {
    macx:BDB_INCLUDE_PATH = /opt/local/include/db48
}

isEmpty(BOOST_LIB_PATH) {
    macx:BOOST_LIB_PATH = /opt/local/lib
}

isEmpty(BOOST_INCLUDE_PATH) {
    macx:BOOST_INCLUDE_PATH = /opt/local/include
}

windows:DEFINES += WIN32 _WIN32
windows:RC_FILE = src/qt/res/bitcoin-qt.rc

windows:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

macx:HEADERS += src/qt/macdockiconhandler.h \
                src/qt/macnotificationhandler.h
macx:OBJECTIVE_SOURCES += src/qt/macdockiconhandler.mm \
                          src/qt/macnotificationhandler.mm
macx:LIBS += -framework Foundation -framework ApplicationServices -framework AppKit
macx:DEFINES += MAC_OSX MSG_NOSIGNAL=0
macx:ICON = src/qt/res/icons/spectre.icns
macx:TARGET = "Spectre"
macx:QMAKE_CFLAGS_THREAD += -pthread
macx:QMAKE_LFLAGS_THREAD += -pthread
macx:QMAKE_CXXFLAGS_THREAD += -pthread

# Set libraries and includes at end, to use platform-defined defaults if not overridden
INCLUDEPATH += $$BOOST_INCLUDE_PATH $$BDB_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(BDB_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,)
LIBS += -lssl -lcrypto -ldb_cxx$$BDB_LIB_SUFFIX
LIBS += -lz -levent
# -lgdi32 has to happen after -lcrypto (see  #681)
windows:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX
windows:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX

contains(RELEASE, 1) {
    !windows:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic
    }
}

!windows:!macx:!android:!ios {
    DEFINES += LINUX
    LIBS += -lrt -ldl
}

system($$QMAKE_LRELEASE -silent $$_PRO_FILE_)
