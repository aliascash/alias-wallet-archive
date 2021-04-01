# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

add_executable(aliaswallet_test test_alias.cpp)
target_sources(aliaswallet_test
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/accounting_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/allocator_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/base32_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/base58_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/base64_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/basic_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/bignum_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/bip32_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/Checkpoints_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/extkey_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/getarg_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/hash_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/hmac_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/key_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/mnemonic_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/mruset_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/multisig_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/netbase_tests.cpp"
#            "${CMAKE_CURRENT_LIST_DIR}/ringsig_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/rpc_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/script_P2SH_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/script_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/sigopcount_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/stealth_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/uint160_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/uint256_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/util_tests.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/wallet_tests.cpp"
        )

target_include_directories(aliaswallet_test
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/.."
        )

target_link_libraries(aliaswallet_test
        aliaswallet_lib
        Boost::unit_test_framework
        )
