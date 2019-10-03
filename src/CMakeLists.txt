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

if(ANDROID)
    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../db5.x
        ${CMAKE_CURRENT_SOURCE_DIR}/../db5.x/build_android
#        ${CMAKE_CURRENT_SOURCE_DIR}/../db5.x/build_vxworks
    )
    target_link_libraries(spectrecoin_lib ${Boost_LIBRARIES})
else()
#    # This is the UI wallet
#    add_executable(Spectrecoin ../spectre.qrc ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
    target_link_libraries(spectrecoin_lib
            ${Boost_LIBRARIES}
            leveldb
            )
endif()

target_include_directories(spectrecoin_lib
        SYSTEM PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/build/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/helpers>
        ${OPENSSL_INCLUDE_DIR}
        )



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

add_subdirectory(json)
add_subdirectory(lz4)
add_subdirectory(wordlists)
add_subdirectory(xxhash)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_subdirectory(win)
endif ()
if (ANDROID)
    message(STATUS "Skipping Qt for Android build")
else()
    add_subdirectory(qt)
endif ()



##############################################################################
##### Setup and link the executables #########################################

# The daemon
add_executable(Spectrecoind ${CMAKE_CURRENT_LIST_DIR}/spectrecoind.cpp)

if (ANDROID)
    # Link the daemon for Android with different include settings
    target_link_libraries(Spectrecoind
            PUBLIC
            spectrecoin_lib
            db5x
            leveldb
            )
else()
    # Link the daemon
    target_link_libraries(Spectrecoind
            PUBLIC
            spectrecoin_lib
            Boost::boost
            Boost::chrono
            Boost::filesystem
            Boost::iostreams
            Boost::program_options
            Boost::system
            Boost::thread
            OpenSSL::SSL
            OpenSSL::Crypto
            Oracle::BerkeleyDB
            leveldb
            )
endif ()
