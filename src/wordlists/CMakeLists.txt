# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

target_sources(aliaswallet_lib
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/chinese_simplified.h"
            "${CMAKE_CURRENT_LIST_DIR}/chinese_traditional.h"
            "${CMAKE_CURRENT_LIST_DIR}/english.h"
            "${CMAKE_CURRENT_LIST_DIR}/french.h"
            "${CMAKE_CURRENT_LIST_DIR}/japanese.h"
            "${CMAKE_CURRENT_LIST_DIR}/spanish.h"
)