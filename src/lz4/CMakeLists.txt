# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

target_sources(spectrecoin_lib
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lz4.h

        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lz4.c
        )
