# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

target_sources(Spectre
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/uritests.h"
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/uritests.cpp"
        )
