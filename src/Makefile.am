AM_LDFLAGS = -Wl,-z,relro -Wl,-z,now
AM_CPPFLAGS = -pthread -O2 \
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

spectrecoind_LDADD = \
						../leveldb/out-static/libmemenv.a \
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
						../tor/src/ext/keccak-tiny/libkeccak-tiny.a

if ENABLE_GUI
qt/moc_%.cpp: qt/%.h
		@MOC@ -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(MOC_CPPFLAGS) $<

qt/ui_%.h: qt/forms/%.ui
		@UIC@ -o $@ $<

qt/qrc_%.cpp: ../%.qrc
		@RCC@ --name spectre -o $@ $<

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
spectre_CPPFLAGS = $(AM_CPPFLAGS) $(QT4_CFLAGS)
endif
