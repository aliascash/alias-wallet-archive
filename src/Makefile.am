AM_LDFLAGS = -Wl,-z,relro -Wl,-z,now
AM_CPPFLAGS =	-pthread -O2 \
		-fstack-protector-all -Wstack-protector \
		-D_FORTIFY_SOURCE \
		-Wall -Wextra -Wno-ignored-qualifiers \
		-Wformat -Wformat-security -Wno-unused-parameter \
		-I../leveldb/include -I../leveldb/helpers \
		-I../db4.8/build_unix
AM_CFLAGS = -std=gnu99 $(AM_CPPFLAGS)

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
						rpcsmessage.cpp \
						rpcextkey.cpp \
						rpcmnemonic.cpp \
						script.cpp \
						sync.cpp \
						util.cpp \
						hash.cpp \
						wallet.cpp \
						walletdb.cpp \
						noui.cpp \
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
						bloom.cpp

bin_PROGRAMS = spectrecoind
spectrecoind_SOURCES = $(common_SOURCES) spectrecoind.cpp

spectrecoind_LDADD =	../leveldb/out-static/libmemenv.a \
			../leveldb/out-static/libleveldb.a \
			../db4.8/build_unix/libdb_cxx.a \
			../tor/src/or/libtor.a \
			../tor/src/common/libor.a \
			../tor/src/common/libor-ctime.a \
			../tor/src/common/libor-crypto.a \
			../tor/src/common/libor-event.a \
			../tor/src/trunnel/libor-trunnel.a \
			../tor/src/common/libcurve25519_donna.a \
			../tor/src/ext/ed25519/donna/libed25519_donna.a \
			../tor/src/ext/ed25519/ref10/libed25519_ref10.a \
			../tor/src/ext/keccak-tiny/libkeccak-tiny.a \
			$(OPENSSL_LIBS) \
			$(LIBEVENT_LIBS) \
			$(LIBSECCOMP_LIBS) \
			$(LIBCAP_LIBS) \
			$(ZLIB_LIBS)

spectrecoind_CPPFLAGS = $(AM_CPPFLAGS) $(OPENSSL_CFLAGS) $(LIBEVENT_CFLAGS) $(LIBSECCOMP_CFLAGS) $(LIBCAP_CFLAGS) $(ZLIB_CFLAGS)
spectrecoind_LDFLAGS = $(AM_LDFLAGS) $(OPENSSL_LDFLAGS) $(LIBEVENT_LDFLAGS) $(LIBSECCOMP_LDFLAGS) $(LIBCAP_LDFLAGS) $(ZLIB_LDFLAGS)

if ENABLE_GUI
qt/qrc_%.cpp: ../%.qrc
		@RCC@ $< -o $@

qt/moc_%.cpp: qt/%.h
		@MOC@ -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(MOC_CPPFLAGS) $<

qt/ui_%.h: qt/forms/%.ui
		@UIC@ -o $@ $<

qt/locale/%.qm: qt/locale/%.ts
		@LRELEASE@ $< -qm $@

BUILT_SOURCES = qt/ui_aboutdialog.h \
								qt/ui_askpassphrasedialog.h \
								qt/ui_coincontroldialog.h \
								qt/ui_editaddressdialog.h \
								qt/ui_rpcconsole.h \
								qt/ui_transactiondescdialog.h

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
									qt/moc_messagemodel.cpp \
									qt/moc_guiutil.cpp \
									qt/moc_aboutdialog.cpp \
									qt/moc_csvmodelwriter.cpp \
									qt/moc_qvaluecombobox.cpp \
									qt/moc_qvalidatedlineedit.cpp \
									qt/moc_bitcoinaddressvalidator.cpp \
									qt/moc_monitoreddatamapper.cpp \
									qt/moc_bitcoinamountfield.cpp \
									qt/locale/umbra_af_ZA.ts \
									qt/locale/umbra_ar.ts \
									qt/locale/umbra_be_BY.ts \
									qt/locale/umbra_bg.ts \
									qt/locale/umbra_bs.ts \
									qt/locale/umbra_ca_ES.ts \
									qt/locale/umbra_ca.ts \
									qt/locale/umbra_ca@valencia.ts \
									qt/locale/umbra_cs.ts \
									qt/locale/umbra_cy.ts \
									qt/locale/umbra_da.ts \
									qt/locale/umbra_de.ts \
									qt/locale/umbra_el_GR.ts \
									qt/locale/umbra_el.ts \
									qt/locale/umbra_en.ts \
									qt/locale/umbra_eo.ts \
									qt/locale/umbra_es_CL.ts \
									qt/locale/umbra_es_DO.ts \
									qt/locale/umbra_es_MX.ts \
									qt/locale/umbra_es.ts \
									qt/locale/umbra_es_UY.ts \
									qt/locale/umbra_et.ts \
									qt/locale/umbra_eu_ES.ts \
									qt/locale/umbra_fa_IR.ts \
									qt/locale/umbra_fa.ts \
									qt/locale/umbra_fi.ts \
									qt/locale/umbra_fr_CA.ts \
									qt/locale/umbra_fr.ts \
									qt/locale/umbra_gl.ts \
									qt/locale/umbra_he.ts \
									qt/locale/umbra_hi_IN.ts \
									qt/locale/umbra_hr.ts \
									qt/locale/umbra_hu.ts \
									qt/locale/umbra_id_ID.ts \
									qt/locale/umbra_it.ts \
									qt/locale/umbra_ja.ts \
									qt/locale/umbra_ka.ts \
									qt/locale/umbra_kk_KZ.ts \
									qt/locale/umbra_ko_KR.ts \
									qt/locale/umbra_ky.ts \
									qt/locale/umbra_la.ts \
									qt/locale/umbra_lt.ts \
									qt/locale/umbra_lv_LV.ts \
									qt/locale/umbra_ms_MY.ts \
									qt/locale/umbra_nb.ts \
									qt/locale/umbra_nl.ts \
									qt/locale/umbra_pam.ts \
									qt/locale/umbra_pl.ts \
									qt/locale/umbra_pt_BR.ts \
									qt/locale/umbra_pt_PT.ts \
									qt/locale/umbra_ro_RO.ts \
									qt/locale/umbra_ru.ts \
									qt/locale/umbra_sah.ts \
									qt/locale/umbra_sk.ts \
									qt/locale/umbra_sl_SI.ts \
									qt/locale/umbra_sq.ts \
									qt/locale/umbra_sr.ts \
									qt/locale/umbra_sv.ts \
									qt/locale/umbra_th_TH.ts \
									qt/locale/umbra_tr.ts \
									qt/locale/umbra_uk.ts \
									qt/locale/umbra_ur_PK.ts \
									qt/locale/umbra_vi.ts \
									qt/locale/umbra_vi_VN.ts \
									qt/locale/umbra_zh_CN.ts \
									qt/locale/umbra_zh_HK.ts \
									qt/locale/umbra_zh_TW.ts \
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
									qt/messagemodel.cpp \
									qt/spectregui.cpp \
									qt/spectre.cpp \
									qt/spectrebridge.cpp
spectre_LDADD = $(spectrecoind_LDADD) $(QT4_LIBS)
spectre_CPPFLAGS = $(spectrecoind_CPPFLAGS) $(QT4_CFLAGS)
spectre_LDFLAGS = $(spectrecoind_LDFLAGS) $(QT4_LDFLAGS)
endif
