AM_LDFLAGS = -fstack-protector

if OS_LINUX
AM_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,noexecheap -Wl,-z,nodlopen -Wl,-z,nodump
else
if OS_WINDOWS_CROSSCOMPILE
AM_LDFLAGS += -Wl,--dynamicbase -Wl,--nxcompat -Wl,--subsystem,windows
else
if OS_MACOS
AM_LDFLAGS += $(MACOS_RPATH)
endif
endif
endif

AM_CPPFLAGS = -pthread -fPIC -fstack-protector -O2 \
	      -D_FORTIFY_SOURCE=1 \
	      -Wall -Wextra -Wno-ignored-qualifiers -Woverloaded-virtual \
	      -Wformat -Wformat-security -Wno-unused-parameter

if OS_MACOS
AM_CPPFLAGS += -std=c++17 -mmacosx-version-min=10.10
endif

common_SOURCES = anonymize.cpp \
		 json/json_spirit_reader.cpp \
		 json/json_spirit_writer.cpp \
		 alert.cpp \
		 version.cpp \
		 checkpoints.cpp \
		 netbase.cpp \
		 addrman.cpp \
		 crypter.cpp \
		 key.cpp \
		 eckey.cpp \
		 extkey.cpp \
		 db.cpp \
		 init.cpp \
		 keystore.cpp \
		 miner.cpp \
		 main.cpp \
		 net.cpp \
		 protocol.cpp \
		 rpcprotocol.cpp \
		 rpcserver.cpp \
		 rpcclient.cpp \
		 rpcdump.cpp \
		 rpcnet.cpp \
		 rpcmining.cpp \
		 rpcwallet.cpp \
		 rpcblockchain.cpp \
		 rpcrawtransaction.cpp \
		 rpcextkey.cpp \
		 rpcmnemonic.cpp \
		 script.cpp \
		 sync.cpp \
		 util.cpp \
		 hash.cpp \
		 wallet.cpp \
		 walletdb.cpp \
		 kernel.cpp \
		 pbkdf2.cpp \
		 scrypt.cpp \
		 scrypt-arm.S \
		 scrypt-x86.S \
		 scrypt-x86_64.S \
		 smessage.cpp \
		 stealth.cpp \
		 ringsig.cpp \
		 core.cpp \
		 txdb-leveldb.cpp \
		 txmempool.cpp \
		 chainparams.cpp \
		 state.cpp \
		 bloom.cpp \
		 shutdown.cpp

bin_PROGRAMS = spectrecoind
spectrecoind_SOURCES = $(common_SOURCES) \
		       spectrecoind.cpp \
		       noui.cpp

spectrecoind_LDADD = ../leveldb/out-static/libmemenv.a \
		     ../leveldb/out-static/libleveldb.a \
		     ../db4.8/build_unix/libdb_cxx.a \
		     $(OPENSSL_LIBS) \
		     $(LIBEVENT_LIBS) \
		     $(LIBSECCOMP_LIBS) \
		     $(LIBCAP_LIBS) \
		     $(ZLIB_LIBS) \
		     $(ZSTD_LIBS) \
		     $(LZMA_LIBS) \
		     $(BOOST_SYSTEM_LIBS) \
		     $(BOOST_CHRONO_LIBS) \
		     $(BOOST_FILESYSTEM_LIBS) \
		     $(BOOST_PROGRAM_OPTIONS_LIBS) \
		     $(BOOST_THREAD_LIBS)

spectrecoind_CPPFLAGS = $(AM_CPPFLAGS) $(OPENSSL_CFLAGS) $(LIBEVENT_CFLAGS) \
			$(LIBSECCOMP_CFLAGS) $(LIBCAP_CFLAGS) $(ZLIB_CFLAGS) $(ZSTD_CPPFLAGS) $(LZMA_CPPFLAGS) \
			$(BOOST_CPPFLAGS) \
			-I../leveldb/include -I../leveldb/helpers -I../db4.8/build_unix \
			-D_FORTIFY_SOURCE=1 -DBOOST_THREAD_USE_LIB -DBOOST_SPIRIT_THREADSAFE \
			-DBOOST_ASIO_ENABLE_OLD_SERVICES
spectrecoind_CFLAGS = $(spectrecoind_CPPFLAGS) -std=c11
spectrecoind_LDFLAGS = $(AM_LDFLAGS) $(OPENSSL_LDFLAGS) $(LIBEVENT_LDFLAGS) \
		       $(LIBSECCOMP_LDFLAGS) $(LIBCAP_LDFLAGS) $(ZLIB_LDFLAGS) $(ZSTD_LDFLAGS) $(LZMA_LDFLAGS) \
		       $(BOOST_SYSTEM_LDFLAGS) $(BOOST_CHRONO_LDFLAGS) $(BOOST_FILESYSTEM_LDFLAGS) \
		       $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_THREAD_LDFLAGS)
spectrecoind_LIBTOOLFLAGS = --tag CXX

if OS_WINDOWS_CROSSCOMPILE
spectrecoind_LDADD += -lmswsock -lshlwapi -luuid -lole32 -lgdi32
spectrecoind_CPPFLAGS += -DWIN32 -D_WIN32
else
if OS_MACOS
spectrecoind_CPPFLAGS += -DMAC_OSX
else
spectrecoind_LDFLAGS += -pie
spectrecoind_LDADD += -lcap -lseccomp
endif
endif

if ENABLE_GUI
qt/locale/%.qm: qt/locale/%.ts
		@LRELEASE@ $< -qm $@

qt/qrc_%.cpp: ../%.qrc
		@RCC@ $< -o $@

if OS_MACOS
  MOC_CPPFLAGS = -DQ_OS_MAC
endif

qt/moc_%.cpp: qt/%.h
		@MOC@ -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(MOC_CPPFLAGS) $<

qt/ui_%.h: qt/forms/%.ui
		@UIC@ -o $@ $<

BUILT_SOURCES = qt/ui_aboutdialog.h \
		qt/ui_askpassphrasedialog.h \
		qt/ui_coincontroldialog.h \
		qt/ui_editaddressdialog.h \
		qt/ui_rpcconsole.h \
		qt/ui_transactiondescdialog.h \
		qt/locale/spectrecoin_af_ZA.qm \
		qt/locale/spectrecoin_ar.qm \
		qt/locale/spectrecoin_be_BY.qm \
		qt/locale/spectrecoin_bg.qm \
		qt/locale/spectrecoin_bs.qm \
		qt/locale/spectrecoin_ca_ES.qm \
		qt/locale/spectrecoin_ca.qm \
		qt/locale/spectrecoin_ca@valencia.qm \
		qt/locale/spectrecoin_cs.qm \
		qt/locale/spectrecoin_cy.qm \
		qt/locale/spectrecoin_da.qm \
		qt/locale/spectrecoin_de.qm \
		qt/locale/spectrecoin_el_GR.qm \
		qt/locale/spectrecoin_el.qm \
		qt/locale/spectrecoin_en.qm \
		qt/locale/spectrecoin_eo.qm \
		qt/locale/spectrecoin_es_CL.qm \
		qt/locale/spectrecoin_es_DO.qm \
		qt/locale/spectrecoin_es_MX.qm \
		qt/locale/spectrecoin_es.qm \
		qt/locale/spectrecoin_es_UY.qm \
		qt/locale/spectrecoin_et.qm \
		qt/locale/spectrecoin_eu_ES.qm \
		qt/locale/spectrecoin_fa_IR.qm \
		qt/locale/spectrecoin_fa.qm \
		qt/locale/spectrecoin_fi.qm \
		qt/locale/spectrecoin_fr_CA.qm \
		qt/locale/spectrecoin_fr.qm \
		qt/locale/spectrecoin_gl.qm \
		qt/locale/spectrecoin_he.qm \
		qt/locale/spectrecoin_hi_IN.qm \
		qt/locale/spectrecoin_hr.qm \
		qt/locale/spectrecoin_hu.qm \
		qt/locale/spectrecoin_id_ID.qm \
		qt/locale/spectrecoin_it.qm \
		qt/locale/spectrecoin_ja.qm \
		qt/locale/spectrecoin_ka.qm \
		qt/locale/spectrecoin_kk_KZ.qm \
		qt/locale/spectrecoin_ko_KR.qm \
		qt/locale/spectrecoin_ky.qm \
		qt/locale/spectrecoin_la.qm \
		qt/locale/spectrecoin_lt.qm \
		qt/locale/spectrecoin_lv_LV.qm \
		qt/locale/spectrecoin_ms_MY.qm \
		qt/locale/spectrecoin_nb.qm \
		qt/locale/spectrecoin_nl.qm \
		qt/locale/spectrecoin_pam.qm \
		qt/locale/spectrecoin_pl.qm \
		qt/locale/spectrecoin_pt_BR.qm \
		qt/locale/spectrecoin_pt_PT.qm \
		qt/locale/spectrecoin_ro_RO.qm \
		qt/locale/spectrecoin_ru.qm \
		qt/locale/spectrecoin_sah.qm \
		qt/locale/spectrecoin_sk.qm \
		qt/locale/spectrecoin_sl_SI.qm \
		qt/locale/spectrecoin_sq.qm \
		qt/locale/spectrecoin_sr.qm \
		qt/locale/spectrecoin_sv.qm \
		qt/locale/spectrecoin_th_TH.qm \
		qt/locale/spectrecoin_tr.qm \
		qt/locale/spectrecoin_uk.qm \
		qt/locale/spectrecoin_ur_PK.qm \
		qt/locale/spectrecoin_vi.qm \
		qt/locale/spectrecoin_vi_VN.qm \
		qt/locale/spectrecoin_zh_CN.qm \
		qt/locale/spectrecoin_zh_HK.qm \
		qt/locale/spectrecoin_zh_TW.qm

bin_PROGRAMS += spectre

spectre_SOURCES = $(common_SOURCES) \
		  qt/qrc_spectre.cpp \
		  qt/moc_spectregui.cpp \
		  qt/moc_spectrebridge.cpp \
		  qt/moc_rpcconsole.cpp \
		  qt/moc_peertablemodel.cpp \
		  qt/moc_notificator.cpp \
		  qt/moc_editaddressdialog.cpp \
		  qt/moc_askpassphrasedialog.cpp \
		  qt/moc_coincontroldialog.cpp \
		  qt/moc_coincontroltreewidget.cpp \
		  qt/moc_addresstablemodel.cpp \
		  qt/moc_transactiontablemodel.cpp \
		  qt/moc_transactiondesc.cpp \
		  qt/moc_trafficgraphwidget.cpp \
		  qt/moc_paymentserver.cpp \
		  qt/moc_clientmodel.cpp \
		  qt/moc_walletmodel.cpp \
		  qt/moc_optionsmodel.cpp \
		  qt/moc_guiutil.cpp \
		  qt/moc_aboutdialog.cpp \
		  qt/moc_csvmodelwriter.cpp \
		  qt/moc_qvaluecombobox.cpp \
		  qt/moc_qvalidatedlineedit.cpp \
		  qt/moc_bitcoinaddressvalidator.cpp \
		  qt/moc_monitoreddatamapper.cpp \
		  qt/moc_bitcoinamountfield.cpp \
		  qt/transactiontablemodel.cpp \
		  qt/coincontroldialog.cpp \
		  qt/coincontroltreewidget.cpp \
		  qt/aboutdialog.cpp \
		  qt/addresstablemodel.cpp \
		  qt/editaddressdialog.cpp \
		  qt/bitcoinaddressvalidator.cpp \
		  qt/clientmodel.cpp \
		  qt/guiutil.cpp \
		  qt/transactionrecord.cpp \
		  qt/optionsmodel.cpp \
		  qt/monitoreddatamapper.cpp \
		  qt/transactiondesc.cpp \
		  qt/bitcoinstrings.cpp \
		  qt/bitcoinamountfield.cpp \
		  qt/walletmodel.cpp \
		  qt/csvmodelwriter.cpp \
		  qt/qvalidatedlineedit.cpp \
		  qt/bitcoinunits.cpp \
		  qt/qvaluecombobox.cpp \
		  qt/askpassphrasedialog.cpp \
		  qt/notificator.cpp \
		  qt/rpcconsole.cpp \
		  qt/paymentserver.cpp \
		  qt/peertablemodel.cpp \
		  qt/scicon.cpp \
		  qt/trafficgraphwidget.cpp \
		  qt/spectregui.cpp \
		  qt/spectre.cpp \
		  qt/spectrebridge.cpp \
		  qt/winshutdownmonitor.cpp

spectre_LDADD = $(spectrecoind_LDADD) $(QT5_LIBS) $(QT5WEBKIT_LIBS)
spectre_CPPFLAGS = $(spectrecoind_CPPFLAGS) $(QT5_CFLAGS) $(QT5WEBKIT_CFLAGS) -DQT_DISABLE_DEPRECATED_BEFORE=0
spectre_LDFLAGS = $(spectrecoind_LDFLAGS) $(QT5_LDFLAGS) $(QT5WEBKIT_LDFLAGS)
spectre_LIBTOOLFLAGS = $(spectrecoind_LIBTOOLFLAGS)

if OS_MACOS
  spectre_SOURCES += qt/moc_macnotificationhandler.cpp \
		     qt/macnotificationhandler.mm \
		     qt/moc_macdockiconhandler.cpp \
		     qt/macdockiconhandler.mm
  spectre_LDFLAGS += -framework Foundation -framework AppKit -framework CoreServices
endif
endif

clean-local:
	rm -f qt/moc_*.cpp
	rm -f qt/ui_*.cpp
	rm -f qt/qrc_*.cpp
