// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef TYPES_H
#define TYPES_H

typedef std::vector<uint8_t> data_chunk;

const size_t EC_SECRET_SIZE = 32;
const size_t EC_COMPRESSED_SIZE = 33;
const size_t EC_UNCOMPRESSED_SIZE = 65;

typedef struct ec_secret { uint8_t e[EC_SECRET_SIZE]; } ec_secret;

typedef data_chunk ec_point;


#endif  // TYPES_H
