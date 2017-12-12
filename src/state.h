// Copyright (c) 2014 The ShadowCoin developers
// Copyright (c) 2016 The SpectreCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_STATE_H
#define COIN_STATE_H

#include <string>
#include <limits>
#include "sync.h"

enum eNodeType
{
    NT_FULL = 1,
    NT_THIN,
    NT_UNKNOWN // end marker
};

enum eNodeState
{
    NS_STARTUP = 1,
    NS_GET_HEADERS,
    NS_GET_FILTERED_BLOCKS,
    NS_READY,
    
    NS_UNKNOWN // end marker
};

enum eBlockFlags
{
    BLOCK_PROOF_OF_STAKE = (1 << 0), // is proof-of-stake block
    BLOCK_STAKE_ENTROPY  = (1 << 1), // entropy bit for stake modifier
    BLOCK_STAKE_MODIFIER = (1 << 2), // regenerated stake modifier
};


/*  nServices flags
    top 32 bits of CNode::nServices are used to mark services required 
*/

enum
{
    NODE_NETWORK = (1 << 0),
    THIN_SUPPORT = (1 << 1),
    THIN_STAKE   = (1 << 2),  // deprecated
    THIN_STEALTH = (1 << 3),
    SMSG_RELAY   = (1 << 4),
};

const int64_t GENESIS_BLOCK_TIME = 1479594600;

static const int64_t COIN = 100000000;
static const int64_t CENT = 1000000;

/** Fees smaller than this (in satoshi) are considered zero fee (for transaction creation) */
static const int64_t MIN_TX_FEE = 10000;
static const int64_t MIN_TX_FEE_ANON = 1000000;
/** Fees smaller than this (in satoshi) are considered zero fee (for relaying) */
static const int64_t MIN_RELAY_TX_FEE = MIN_TX_FEE;
static const int64_t COIN_YEAR_REWARD = 5 * CENT; // 5% per year

static const int64_t MBLK_RECEIVE_TIMEOUT = 60; // seconds

extern int nNodeMode;
extern int nNodeState;

extern int nMaxThinPeers;
extern int nBloomFilterElements;

extern int nMinStakeInterval;
extern int nStakingDonation;

extern int nThinIndexWindow;

static const int nTryStakeMempoolTimeout = 5 * 60; // seconds
static const int nTryStakeMempoolMaxAsk = 16;

extern uint64_t nLocalServices;
extern uint32_t nLocalRequirements;




extern bool fTestNet;
extern bool fDebug;
extern bool fDebugNet;
extern bool fDebugSmsg;
extern bool fDebugChain;
extern bool fDebugRingSig;
extern bool fDebugPoS;
extern bool fNoSmsg;
extern bool fPrintToConsole;
extern bool fPrintToDebugLog;
//extern bool fShutdown;
extern bool fDaemon;
extern bool fServer;
extern bool fCommandLine;
extern std::string strMiscWarning;
extern bool fNoListen;
extern bool fLogTimestamps;
extern bool fReopenDebugLog;
extern bool fThinFullIndex;
extern bool fReindexing;
extern bool fHaveGUI;
extern volatile bool fIsStaking;
extern bool fMakeExtKeyInitials;
extern volatile bool fPassGuiAddresses;

extern bool fConfChange;
extern bool fEnforceCanonical;
extern unsigned int nNodeLifespan;
extern unsigned int nDerivationMethodIndex;
extern unsigned int nMinerSleep;
extern unsigned int nBlockMaxSize;
extern unsigned int nBlockPrioritySize;
extern unsigned int nBlockMinSize;
extern int64_t nMinTxFee;

extern unsigned int nStakeSplitAge;
extern int nStakeMinConfirmations;
extern int64_t nStakeSplitThreshold;
extern int64_t nStakeCombineThreshold;

extern uint32_t nExtKeyLookAhead;
extern int64_t nTimeLastMblkRecv;


#endif /* COIN_STATE_H */

