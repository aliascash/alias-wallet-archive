// SPDX-FileCopyrightText: © 2013 Balthazar
// SPDX-FileCopyrightText: © 2011 pooler
// SPDX-FileCopyrightText: © 2011 ArtForz
// SPDX-FileCopyrightText: © 2009 Colin Percival
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef SCRYPT_MINE_H
#define SCRYPT_MINE_H

#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "net.h"

uint256 scrypt_salted_multiround_hash(const void* input, size_t inputlen, const void* salt, size_t saltlen, const unsigned int nRounds);
uint256 scrypt_salted_hash(const void* input, size_t inputlen, const void* salt, size_t saltlen);
uint256 scrypt_hash(const void* input, size_t inputlen);
uint256 scrypt_blockhash(const void* input);

#endif // SCRYPT_MINE_H
