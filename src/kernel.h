// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2012 PPCoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef PPCOIN_KERNEL_H
#define PPCOIN_KERNEL_H

#include "main.h"
#include "core.h"

// To decrease granularity of timestamp
// Supposed to be 2^n-1
static const int STAKE_TIMESTAMP_MASK = 15;

// MODIFIER_INTERVAL: time to elapse before new modifier is computed
extern unsigned int nModifierInterval;

// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static const int MODIFIER_INTERVAL_RATIO = 3;

// Compute the hash modifier for proof-of-stake
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);
bool ComputeNextStakeModifierThin(const CBlockThinIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);
uint256 ComputeStakeModifierV2(const CBlockIndex* pindexPrev, const uint256& kernel);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckStakeKernelHash(int nPrevHeight, CStakeModifier* pStakeMod, unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(CBlockIndex* pindexPrev, const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, uint256& targetProofOfStake);

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int nHeight, int64_t nTimeBlock, int64_t nTimeTx);

// Get time weight using supplied timestamps
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd); // TODO: posv2 remove

// Wrapper around CheckStakeKernelHash()
// Also checks existence of kernel input and min age
// Convenient for searching a kernel
bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime = NULL);


// -- Stealth Staking
// Check whether stake kernel meets hash target and ATXO maturity
// Sets hashProofOfStake on success return
bool CheckAnonProofOfStake(CBlockIndex* pindexPrev, const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, uint256& targetProofOfStake);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckAnonStakeKernelHash(CStakeModifier* pStakeMod, const unsigned int& nBits, const int64_t& anonValue, const ec_point &anonKeyImage,  const unsigned int& nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, const bool fPrintProofOfStake);

// Wrapper around CheckAnonStakeKernelHash()
// Convenient for searching a anon kernel
bool CheckAnonKernel(const CBlockIndex* pindexPrev, const unsigned int& nBits, const int64_t& anonValue, const ec_point& anonKeyImage, const unsigned int& nTime);

#endif // PPCOIN_KERNEL_H
