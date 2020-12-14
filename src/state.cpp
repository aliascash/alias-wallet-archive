// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
//
// SPDX-License-Identifier: MIT

#include "state.h"

int nNodeMode = NT_FULL;
int nNodeState = NS_STARTUP;

int nMaxThinPeers = 8;
int nBloomFilterElements = 1536;
int nMinStakeInterval = 0;         // in seconds, min time between successful stakes
int nStakingDonation = 0;          // percentage of stakes that will be donated to the developers
int nThinIndexWindow = 4096;        // no. of block headers to keep in memory

// -- services provided by local node, initialise to all on
uint64_t nLocalServices     = 0 | NODE_NETWORK | THIN_SUPPORT | THIN_STEALTH | SMSG_RELAY;
uint32_t nLocalRequirements = 0 | NODE_NETWORK;


bool fTestNet = false;
bool fDebug = false;
bool fDebugNet = false;
bool fDebugChain = false;
bool fDebugRingSig = false;
bool fDebugPoS = false;
bool fPrintToConsole = false;
bool fPrintToDebugLog = true;
//bool fShutdown = false;
bool fDaemon = false;
bool fServer = false;
bool fCommandLine = false;
std::string strMiscWarning;
bool fNoListen = false;
bool fLogTimestamps = false;
bool fReopenDebugLog = false;
bool fThinFullIndex = false; // when in thin mode don't keep all headers in memory
bool fReindexing = false;
bool fHaveGUI = false;
volatile bool fIsStaking = false; // looks at stake weight also
volatile bool fIsStakingEnabled = false;
bool fMakeExtKeyInitials = false;
volatile bool fPassGuiAddresses = false; // force the gui to process new addresses, gui doesn't update addresses when syncing

bool fConfChange;
bool fEnforceCanonical;
bool fUseFastIndex;
unsigned int nNodeLifespan;
unsigned int nDerivationMethodIndex;
unsigned int nMinerSleep;
unsigned int nBlockMaxSize;
unsigned int nBlockPrioritySize;
unsigned int nBlockMinSize;

int64_t nMinTxFee = 10000;
int64_t nMinRelayTxFee = nMinTxFee;

int64_t nStakeReward = 2 * COIN;
int64_t nAnonStakeReward = 3 * COIN;

unsigned int nStakeSplitAge = 1 * 24 * 60 * 60;
int64_t nStakeCombineThreshold = 1000 * COIN;
int64_t nStakeSplitThreshold = 2 * nStakeCombineThreshold;
int64_t nMaxAnonOutput = 1000 * COIN;
int64_t nMaxAnonStakeOutput = nMaxAnonOutput;

uint32_t nExtKeyLookAhead = 10;

int64_t nTimeLastMblkRecv = 0;

std::atomic<CoreRunningMode> coreRunningMode(CoreRunningMode::RUNNING_NORMAL);
CCriticalSection cs_service;

