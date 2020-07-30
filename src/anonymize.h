// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2013 BritCoin Developers
//
// SPDX-License-Identifier: MIT

/**
 * \file anonymize.h
 * \brief Headers for anonymize.cpp
 **/

#ifndef TOR_ANONYMIZE_H
#define TOR_ANONYMIZE_H

#ifdef __cplusplus
extern "C" {
#endif

    char const* anonymize_tor_data_directory(void);
    char const* anonymize_service_directory(void);

#ifdef __cplusplus
}
#endif

#endif

