# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT

target_sources(spectrecoin_lib
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_error_position.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_reader.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_reader_template.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_stream_reader.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_utils.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_value.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_writer.h
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_writer_template.h

        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_reader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_value.cpp
        ${CMAKE_CURRENT_LIST_DIR}/json_spirit_writer.cpp
        )
