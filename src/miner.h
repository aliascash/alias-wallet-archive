// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2013 NovaCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
// SPDX-FileCopyrightText: © 2009 Satoshi Nakamoto
//
// SPDX-License-Identifier: MIT

#ifndef NOVACOIN_MINER_H
#define NOVACOIN_MINER_H

#include "main.h"
#include "wallet.h"
#include "init.h"

void ThreadStakeMiner(CWallet *pwallet);

/* Generate a new block, without valid proof-of-work */
CBlock* CreateNewBlock(CWallet* pwallet, bool fProofOfStake=false, int64_t* pFees = 0);

/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);

/** Do mining precalculation */
void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1);

/** Check mined proof-of-work block */
bool CheckWork(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey);

/** Check mined proof-of-stake block */
bool CheckStake(CBlock* pblock, CWallet& wallet);

/** Base sha256 mining transform */
void SHA256Transform(void* pstate, void* pinput, const void* pinit);

#endif // NOVACOIN_MINER_H
