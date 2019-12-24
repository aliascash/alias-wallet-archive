##############################################################################
##### Some helper stuff ######################################################

# Configure a header file to pass some of the CMake settings
# to the source code like version and Git hash
#configure_file(
#        "${CMAKE_CURRENT_LIST_DIR}/build.h.in"
#        "${CMAKE_CURRENT_LIST_DIR}/build.h"
#)
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/clientversion.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/clientversion.h"
)


##############################################################################
##### Setup a library with all the common content ############################

# Create dummy file since we want an empty shared library before linking
set(DUMMY_SOURCE ${CMAKE_BINARY_DIR}/dummy.c)
file(WRITE ${DUMMY_SOURCE} "")

add_library(spectrecoin_lib ${DUMMY_SOURCE})

target_link_libraries(spectrecoin_lib
        bdb
        leveldb
        ssl
        crypto
        Boost::atomic
        Boost::boost
        Boost::chrono
        Boost::date_time
        Boost::filesystem
        Boost::iostreams
        Boost::program_options
        Boost::regex
        Boost::system
        Boost::thread
        )

target_include_directories(spectrecoin_lib
        SYSTEM BEFORE PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/build/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/helpers>
        ${OPENSSL_INCLUDE_PATH}
        ${BERKELEYDB_INCLUDE_PATH}
        )

if (WITH_TOR)
    target_link_libraries(spectrecoin_lib
            libtor
            )

    target_include_directories(spectrecoin_lib
            SYSTEM BEFORE PUBLIC
            ${TOR_INCLUDE_PATH}
            )
endif()

# Common sources
target_sources(spectrecoin_lib
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/addrman.h
        ${CMAKE_CURRENT_LIST_DIR}/alert.h
        ${CMAKE_CURRENT_LIST_DIR}/allocators.h
        ${CMAKE_CURRENT_LIST_DIR}/anonymize.h
        ${CMAKE_CURRENT_LIST_DIR}/base58.h
        ${CMAKE_CURRENT_LIST_DIR}/bignum.h
        ${CMAKE_CURRENT_LIST_DIR}/bloom.h
        ${CMAKE_CURRENT_LIST_DIR}/chainparams.h
        ${CMAKE_CURRENT_LIST_DIR}/chainparamsseeds.h
        ${CMAKE_CURRENT_LIST_DIR}/checkpoints.h
        ${CMAKE_CURRENT_LIST_DIR}/clientversion.h
        ${CMAKE_CURRENT_LIST_DIR}/coincontrol.h
        ${CMAKE_CURRENT_LIST_DIR}/compat.h
        ${CMAKE_CURRENT_LIST_DIR}/core.h
        ${CMAKE_CURRENT_LIST_DIR}/crypter.h
        ${CMAKE_CURRENT_LIST_DIR}/db.h
        ${CMAKE_CURRENT_LIST_DIR}/eckey.h
        ${CMAKE_CURRENT_LIST_DIR}/extkey.h
        ${CMAKE_CURRENT_LIST_DIR}/hash.h
        ${CMAKE_CURRENT_LIST_DIR}/init.h
        ${CMAKE_CURRENT_LIST_DIR}/kernel.h
        ${CMAKE_CURRENT_LIST_DIR}/key.h
        ${CMAKE_CURRENT_LIST_DIR}/keystore.h
        ${CMAKE_CURRENT_LIST_DIR}/main.h
        ${CMAKE_CURRENT_LIST_DIR}/miner.h
        ${CMAKE_CURRENT_LIST_DIR}/mruset.h
        ${CMAKE_CURRENT_LIST_DIR}/netbase.h
        ${CMAKE_CURRENT_LIST_DIR}/net.h
        ${CMAKE_CURRENT_LIST_DIR}/pbkdf2.h
        ${CMAKE_CURRENT_LIST_DIR}/protocol.h
        ${CMAKE_CURRENT_LIST_DIR}/resource.h
        ${CMAKE_CURRENT_LIST_DIR}/ringsig.h
        ${CMAKE_CURRENT_LIST_DIR}/rpcclient.h
        ${CMAKE_CURRENT_LIST_DIR}/rpcprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/rpcserver.h
        ${CMAKE_CURRENT_LIST_DIR}/script.h
        ${CMAKE_CURRENT_LIST_DIR}/scrypt.h
        ${CMAKE_CURRENT_LIST_DIR}/serialize.h
        ${CMAKE_CURRENT_LIST_DIR}/smessage.h
        ${CMAKE_CURRENT_LIST_DIR}/state.h
        ${CMAKE_CURRENT_LIST_DIR}/stealth.h
        ${CMAKE_CURRENT_LIST_DIR}/strlcpy.h
        ${CMAKE_CURRENT_LIST_DIR}/sync.h
        ${CMAKE_CURRENT_LIST_DIR}/threadsafety.h
        ${CMAKE_CURRENT_LIST_DIR}/tinyformat.h
        ${CMAKE_CURRENT_LIST_DIR}/txdb.h
        ${CMAKE_CURRENT_LIST_DIR}/txdb-leveldb.h
        ${CMAKE_CURRENT_LIST_DIR}/txmempool.h
        ${CMAKE_CURRENT_LIST_DIR}/types.h
        ${CMAKE_CURRENT_LIST_DIR}/interface.h
        ${CMAKE_CURRENT_LIST_DIR}/uint256.h
        ${CMAKE_CURRENT_LIST_DIR}/util.h
        ${CMAKE_CURRENT_LIST_DIR}/version.h
        ${CMAKE_CURRENT_LIST_DIR}/walletdb.h
        ${CMAKE_CURRENT_LIST_DIR}/wallet.h
        ${CMAKE_CURRENT_LIST_DIR}/shutdown.h

        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/addrman.cpp
        ${CMAKE_CURRENT_LIST_DIR}/alert.cpp
        ${CMAKE_CURRENT_LIST_DIR}/anonymize.cpp
        ${CMAKE_CURRENT_LIST_DIR}/bloom.cpp
        ${CMAKE_CURRENT_LIST_DIR}/chainparams.cpp
        ${CMAKE_CURRENT_LIST_DIR}/checkpoints.cpp
        ${CMAKE_CURRENT_LIST_DIR}/core.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crypter.cpp
        ${CMAKE_CURRENT_LIST_DIR}/db.cpp
        ${CMAKE_CURRENT_LIST_DIR}/eckey.cpp
        ${CMAKE_CURRENT_LIST_DIR}/extkey.cpp
        ${CMAKE_CURRENT_LIST_DIR}/hash.cpp
        ${CMAKE_CURRENT_LIST_DIR}/init.cpp
        ${CMAKE_CURRENT_LIST_DIR}/kernel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/key.cpp
        ${CMAKE_CURRENT_LIST_DIR}/keystore.cpp
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/miner.cpp
        ${CMAKE_CURRENT_LIST_DIR}/netbase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/net.cpp
        ${CMAKE_CURRENT_LIST_DIR}/noui.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pbkdf2.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ringsig.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcblockchain.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcclient.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcdump.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcextkey.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcmining.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcmnemonic.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcnet.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcrawtransaction.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcserver.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpcwallet.cpp
        ${CMAKE_CURRENT_LIST_DIR}/script.cpp
        ${CMAKE_CURRENT_LIST_DIR}/scrypt.cpp
        ${CMAKE_CURRENT_LIST_DIR}/smessage.cpp
        ${CMAKE_CURRENT_LIST_DIR}/state.cpp
        ${CMAKE_CURRENT_LIST_DIR}/stealth.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sync.cpp
        ${CMAKE_CURRENT_LIST_DIR}/txdb-leveldb.cpp
        ${CMAKE_CURRENT_LIST_DIR}/txmempool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/util.cpp
        ${CMAKE_CURRENT_LIST_DIR}/version.cpp
        ${CMAKE_CURRENT_LIST_DIR}/wallet.cpp
        ${CMAKE_CURRENT_LIST_DIR}/walletdb.cpp
        ${CMAKE_CURRENT_LIST_DIR}/shutdown.cpp
        )

if (EXISTS ${BERKELEYDB_INCLUDE_PATH}/db_cxx.h)
    target_sources(spectrecoin_lib
        PUBLIC
        ${BERKELEYDB_INCLUDE_PATH}/db_cxx.h
    )
else()
    message(STATUS "===> BerkeleyDB: ${BERKELEYDB_INCLUDE_PATH}/db_cxx.h not found. Please re-run cmake after BerkeleyDB was build.")
endif()

if (EXISTS ${OPENSSL_INCLUDE_PATH}/openssl/crypto.h)
    target_sources(spectrecoin_lib
        PUBLIC
        ${OPENSSL_INCLUDE_PATH}/openssl/crypto.h
    )
else()
    message(STATUS "===> OpenSSL: ${OPENSSL_INCLUDE_PATH}/openssl/crypto.h not found. Please re-run cmake after OpenSSL was build.")
endif()

add_subdirectory(json)
add_subdirectory(lz4)
add_subdirectory(wordlists)
add_subdirectory(xxhash)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_subdirectory(win)
endif ()



##############################################################################
##### Setup and link the executables #########################################

# The daemon
add_executable(Spectrecoind ${CMAKE_CURRENT_LIST_DIR}/spectrecoind.cpp)

# Link the daemon as static binary
# ============================================================================================
if (WITH_TOR)
    target_link_libraries(Spectrecoind
            ssl
            crypto
            bdb
            leveldb
            spectrecoin_lib
            libevent
            libzstd
            libxz
            libtor
            -static
            )
else()
    target_link_libraries(Spectrecoind
            ssl
            crypto
            bdb
            leveldb
            spectrecoin_lib
            -static
            )
endif()

if (ANDROID)
    message(STATUS "Skipping Qt for Android build")
else()
    # The UI wallet
    add_executable(Spectrecoin ../spectre.qrc ${CMAKE_CURRENT_LIST_DIR}/qt/spectre.cpp)

    # Link the UI wallet
    # QtWebEngine not static available, so no static binary here :-(
    target_link_libraries(Spectrecoin
            ssl
            crypto
            bdb
            leveldb
            spectrecoin_lib
            Qt5::Core
            Qt5::Widgets
            Qt5::WebEngine
            Qt5::WebEngineWidgets
            )

    target_sources(Spectrecoin
            PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/qt/aboutdialog.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/addresstablemodel.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/askpassphrasedialog.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinaddressvalidator.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinamountfield.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinunits.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/bridgetranslations.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/clientmodel.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/coincontroldialog.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/coincontroltreewidget.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/csvmodelwriter.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/editaddressdialog.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/guiconstants.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/guiutil.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/monitoreddatamapper.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/notificator.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/optionsmodel.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/paymentserver.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/peertablemodel.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/qvalidatedlineedit.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/qvaluecombobox.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/rpcconsole.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/scicon.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/setupwalletwizard.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/spectrebridge.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/spectregui.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/trafficgraphwidget.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/transactiondesc.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/transactionrecord.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/transactiontablemodel.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/walletmodel.h

            PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/qt/aboutdialog.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/addresstablemodel.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/askpassphrasedialog.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinaddressvalidator.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinamountfield.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinstrings.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/bitcoinunits.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/clientmodel.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/coincontroldialog.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/coincontroltreewidget.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/csvmodelwriter.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/editaddressdialog.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/guiutil.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/monitoreddatamapper.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/notificator.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/optionsmodel.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/paymentserver.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/peertablemodel.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/qvalidatedlineedit.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/qvaluecombobox.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/rpcconsole.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/scicon.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/setupwalletwizard.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/spectrebridge.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/spectregui.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/trafficgraphwidget.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/transactiondesc.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/transactionrecord.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/transactiontablemodel.cpp
            ${CMAKE_CURRENT_LIST_DIR}/qt/walletmodel.cpp
            )

    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        target_sources(Spectrecoin
            PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/qt/macdockiconhandler.h
            ${CMAKE_CURRENT_LIST_DIR}/qt/macnotificationhandler.h
            )
    endif()
endif ()

message(STATUS "BERKELEYDB_INCLUDE_PATH: ${BERKELEYDB_INCLUDE_PATH}")
message(STATUS "OPENSSL_INCLUDE_PATH: ${OPENSSL_INCLUDE_PATH}")
message(STATUS "OPENSSL_API_COMPAT: ${OPENSSL_API_COMPAT}")
if (WITH_TOR)
    message(STATUS "LIBEVENT_INCLUDE_PATH: ${LIBEVENT_INCLUDE_PATH}")
    message(STATUS "LIBZ_INCLUDE_PATH: ${LIBZ_INCLUDE_PATH}")
    message(STATUS "LIBXZ_INCLUDE_PATH: ${LIBXZ_INCLUDE_PATH}")
    message(STATUS "TOR_INCLUDE_PATH: ${TOR_INCLUDE_PATH}")
endif ()
