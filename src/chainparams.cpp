// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

int64_t CChainParams::GetProofOfWorkReward(int nHeight, int64_t nFees) const
{
    // miner's coin base reward
    int64_t nSubsidy = 0;

    if(nHeight == 1)
        nSubsidy = (NetworkID() == CChainParams::TESTNET ? 2000000 : 20000000) * COIN;  // 20Mill Pre-mine on MainNet, 2Mill pre-mine on TestNet

    else if(nHeight <= nLastPOWBlock)
        nSubsidy = 0;

    if (fDebug && GetBoolArg("-printcreation"))
        LogPrintf("GetProofOfWorkReward() : create=%s nSubsidy=%d\n", FormatMoney(nSubsidy).c_str(), nSubsidy);

    return nSubsidy;
};


int64_t CChainParams::GetProofOfStakeReward(const CBlockIndex* pindexPrev, int64_t nCoinAge, int64_t nFees) const
{
    // miner's coin stake reward based on coin age spent (coin-days)
    int64_t nSubsidy;

    if (IsForkV4SupplyIncrease(pindexPrev))
        nSubsidy = (NetworkID() == CChainParams::TESTNET ? 300000 : 3000000) * COIN;
    else if (IsProtocolV3(pindexPrev->nHeight))
        nSubsidy = Params().IsForkV3(pindexPrev->GetBlockTime()) ?
                    nStakeReward :
                    (pindexPrev->nMoneySupply / COIN) * COIN_YEAR_REWARD / (365 * 24 * (60 * 60 / 64));
    else
        nSubsidy = nCoinAge * COIN_YEAR_REWARD * 33 / (365 * 33 + 8);

    if (fDebug && GetBoolArg("-printcreation"))
    {
        if (IsProtocolV3(pindexPrev->nHeight))
            LogPrintf("GetProofOfStakeReward(): create=%s\n", FormatMoney(nSubsidy).c_str());
        else
            LogPrintf("GetProofOfStakeReward(): create=%s nCoinAge=%d\n", FormatMoney(nSubsidy).c_str(), nCoinAge);
    }

    return nSubsidy + nFees;
}

int64_t CChainParams::GetProofOfAnonStakeReward(const CBlockIndex* pindexPrev, int64_t nFees) const
{
    int64_t nSubsidy = nAnonStakeReward;
    if (IsForkV4SupplyIncrease(pindexPrev))
        nSubsidy = (NetworkID() == CChainParams::TESTNET ? 300000 : 3000000) * COIN;

    if (fDebug && GetBoolArg("-printcreation"))
        LogPrintf("GetProofOfAnonStakeReward(): create=%s\n", FormatMoney(nSubsidy).c_str());

    return nSubsidy + nFees;
}

bool CChainParams::IsForkV4SupplyIncrease(const CBlockIndex* pindexPrev) const
{
    return pindexPrev->GetBlockTime() >= nForkV4Time && pindexPrev->pprev->GetBlockTime() < nForkV4Time;
}

//
// Main network
//

// Convert the pnSeeds array into usable address objects.
static void convertSeeds(std::vector<CAddress> &vSeedsOut, const unsigned int *data, unsigned int count, int port)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int k = 0; k < count; ++k)
    {
        struct in_addr ip;
        unsigned int i = data[k], t;

        // -- convert to big endian
        t =   (i & 0x000000ff) << 24u
            | (i & 0x0000ff00) << 8u
            | (i & 0x00ff0000) >> 8u
            | (i & 0xff000000) >> 24u;

        memcpy(&ip, &t, sizeof(ip));

        CAddress addr(CService(ip, port));
        addr.nTime = GetTime()-GetRand(nOneWeek)-nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

class CBaseChainParams : public CChainParams {
public:
    CBaseChainParams() {
        const char* pszTimestamp = "https://www.cryptocoinsnews.com/encrypted-services-exec-bitcoins-price-history-follows-gartners-hype-cycle/";
        CTransaction txNew;
        txNew.nTime = GENESIS_BLOCK_TIME;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 0 << CBigNum(42) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].SetEmpty();
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = GENESIS_BLOCK_TIME;

        vSeeds.push_back(CDNSSeedData("node1.spectreproject.io", "node1.spectreproject.io"));
        vSeeds.push_back(CDNSSeedData("node2.spectreproject.io", "node2.spectreproject.io"));
        vSeeds.push_back(CDNSSeedData("node3.spectreproject.io", "node3.spectreproject.io"));
        vSeeds.push_back(CDNSSeedData("node4.spectreproject.io", "node4.spectreproject.io"));
      }
    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual const std::vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    std::vector<CAddress> vFixedSeeds;
};

class CMainParams : public CBaseChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xb5;
        pchMessageStart[1] = 0x22;
        pchMessageStart[2] = 0x5c;
        pchMessageStart[3] = 0xd3;

        vAlertPubKey = ParseHex("04f7bbad03208ea942e292080854d422d046d457949ea70ad3306438fc8357343dccaa73e52291ebe07de85c6701d88d87af2d29c2e3b024fb0ad53f045a6d3ad6");

        nDefaultPort = 37347;
        nRPCPort = 36657;
        nBIP44ID = 0x800000d5;

        //nLastPOWBlock = 2016; // Running for 1 Week after ICO
        nLastPOWBlock = 17000;
        nFirstPosv2Block = 17001;
        nFirstPosv3Block = 17010;

        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20); // "standard" scrypt target limit for proof of work, results with 0,000244140625 proof-of-work difficulty
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 20);
        bnProofOfStakeLimitV2 = CBigNum(~uint256(0) >> 48);

        nStakeMinConfirmationsLegacy = 288;
        nStakeMinConfirmations = 450; // block time 96 seconds * 450 = 12 hours

        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 715015;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x000001fd6111f0d71d90b7d8c827c6028dbc867f6c527d90794a0d22f68fecd4"));
        assert(genesis.hashMerkleRoot == uint256("0x48d79d88cdf7d5c84dbb2ffb4fcaab253cebe040a4e7b46cdd507fbb93623e3f"));

        base58Prefixes[PUBKEY_ADDRESS]      = list_of(63).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SCRIPT_ADDRESS]      = list_of(136).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SECRET_KEY]          = list_of(179).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[STEALTH_ADDRESS]     = list_of(40).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_PUBLIC_KEY]      = list_of(0x2C)(0x51)(0x3B)(0xD7).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY]      = list_of(0x2C)(0x51)(0xC1)(0x5A).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_KEY_HASH]        = list_of(137).convert_to_container<std::vector<unsigned char> >();         // x
        base58Prefixes[EXT_ACC_HASH]        = list_of(83).convert_to_container<std::vector<unsigned char> >();          // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC]  = list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >(); // xprv
        base58Prefixes[EXT_SECRET_KEY_BTC]  = list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >(); // xpub

        convertSeeds(vFixedSeeds, pnSeed, ARRAYLEN(pnSeed), nDefaultPort);

        nForkV2Time = 1534888800; // MAINNET V2 chain fork (GMT: Tuesday, 21. August 2018 22.00)
        nForkV3Time = 1558123200; // MAINNET V3 chain fork (GMT: Friday, 17. May 2019 20:00:00)
        nForkV4Time = 1569614400; // MAINNET V4 chain fork (GMT: Friday, 27. September 2019 20:00:00)

        devContributionAddress = "SdrdWNtjD7V6BSt3EyQZKCnZDkeE28cZhr";
        supplyIncreaseAddress = "SSGCEMb6xESgmuGXkx7yozGDxhVSXzBP3a";
    }

    virtual Network NetworkID() const { return CChainParams::MAIN; }
};
static CMainParams mainParams;

//
// Testnet
//

class CTestNetParams : public CBaseChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        strDataDir = "testnet";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xa3;
        pchMessageStart[1] = 0x2c;
        pchMessageStart[2] = 0x44;
        pchMessageStart[3] = 0xb4;

        vAlertPubKey = ParseHex("04e564bc9bf28e6d395cd89c4d2bdb235c3873f59b1330d2e6a30c6fa85d8a8637693ae367ce39c2fe0f4e8e3c7c3a34feb82305388f19030aa4fcd4955abeb810");

        nDefaultPort = 37111;
        nRPCPort = 36757;
        nBIP44ID = 0x80000001;

        nLastPOWBlock = 20;
        nFirstPosv2Block = 20;
        nFirstPosv3Block = 500;

        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 20);
        bnProofOfStakeLimitV2 = CBigNum(~uint256(0) >> 46);

        nStakeMinConfirmationsLegacy = 28;
        nStakeMinConfirmations = 30;

        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 20;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0a3e03a153b1713ebc1f03fefa5d013bba4d2677ae189fcb727396b98043d95c"));

        base58Prefixes[PUBKEY_ADDRESS]      = list_of(127).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SCRIPT_ADDRESS]      = list_of(196).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SECRET_KEY]          = list_of(255).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[STEALTH_ADDRESS]     = list_of(40).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_PUBLIC_KEY]      = list_of(0x76)(0xC0)(0xFD)(0xFB).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY]      = list_of(0x76)(0xC1)(0x07)(0x7A).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_KEY_HASH]        = list_of(75).convert_to_container<std::vector<unsigned char> >();          // X
        base58Prefixes[EXT_ACC_HASH]        = list_of(23).convert_to_container<std::vector<unsigned char> >();          // A
        base58Prefixes[EXT_PUBLIC_KEY_BTC]  = list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >(); // tprv
        base58Prefixes[EXT_SECRET_KEY_BTC]  = list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >(); // tpub

        convertSeeds(vFixedSeeds, pnTestnetSeed, ARRAYLEN(pnTestnetSeed), nDefaultPort);

        nForkV2Time = 1532466000; // TESTNET V2 chain fork (GMT: Tuesday, 24. July 2018 21.00)
        nForkV3Time = 1546470000; // TESTNET V3 chain fork (01/02/2019 @ 11:00pm (UTC))
        nForkV4Time = 1567972800; // TESTNET V4 chain fork (Sunday, 8. September 2019 20:00:00)


        devContributionAddress = "tSJoPZoXumJyDmGKYo9Y7SZkJvymESFYkD";
        supplyIncreaseAddress = devContributionAddress;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;


//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        strDataDir = "regtest";

        nFirstPosv2Block = -1;
        nFirstPosv3Block = -1;

        pchMessageStart[0] = 0x05;
        pchMessageStart[1] = 0xc5;
        pchMessageStart[2] = 0x04;
        pchMessageStart[3] = 0x3a;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1479594600;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 2;

        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;

        assert(hashGenesisBlock == uint256("0x562dba63b74b056329585b9779306f3d3caf447b5df40fb088cebbfb31fd5d5d"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

const CChainParams &TestNetParams() {
    return testNetParams;
}

const CChainParams &MainNetParams() {
    return mainParams;
}

void SelectParams(CChainParams::Network network)
{
    switch (network)
    {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    };
};

bool SelectParamsFromCommandLine()
{
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest)
    {
        return false;
    };

    if (fRegTest)
    {
        SelectParams(CChainParams::REGTEST);
    } else
    if (fTestNet)
    {
        SelectParams(CChainParams::TESTNET);
    } else
    {
        SelectParams(CChainParams::MAIN);
    };

    return true;
}
