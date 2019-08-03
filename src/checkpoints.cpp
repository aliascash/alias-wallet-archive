// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "txdb.h"
#include "main.h"
#include "uint256.h"


static const int nCheckpointSpan = 500;

namespace Checkpoints
{

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        ( 0,        uint256("0x000001fd6111f0d71d90b7d8c827c6028dbc867f6c527d90794a0d22f68fecd4") ) // Params().HashGenesisBlock())
        ( 10000,    uint256("0x89fa592395c4fff97e04ade632dfc33bee19fc46d489abf51331ee68a20ad19d") )
        ( 15000,    uint256("0xb54f2cc3bc31a0f7110ef5f95a1c00744793f5419b1d43044fe393a777edca39") )
        ( 20000,    uint256("0x87294db40d1af9101876293140cb2d82aef893881fbdb2a48ef0b0e062b6966a") )
        ( 25000,    uint256("0x70aaaf2d6dd706fa4689a5e6e17994342e661ea37f72333e63534cf0cc3d7dc1") )
        ( 100000,   uint256("0x4f61bb5f1b22065242784fdb0232ed6ef66ec3e5049d412038f4753dca931cd1") )
        ( 200000,   uint256("0x62f50e7c60480ded7aceed530a6d2f2b204647443425316e5f5714da489d55e3") )
        ( 300000,   uint256("0xe355a6acab9a5a8ae5f5ba0336e3efd40145726106dd1a33a314a86a1dfb8fca") )
        ( 400000,   uint256("0xe09449a9923e5f99d224fbc088ae5c79c6fa2caf511c2a702c27d2afa570b9b7") )
        ( 500000,   uint256("0x3797b3133f61859680141058755cc8224db7b17f6c06f9228b7e2fd7b7e48cf5") )
        ( 600000,   uint256("0x6b8e1852d66a954e94ee85b239ca1a04ee18f676e8ed6b9980f2eb0ccfa0ca9e") )
        ( 700000,   uint256("0x501b1398372508e8806f49de18a579d482895331b958dda932f0ec451a590497") )
        ( 800000,   uint256("0xa502c68dc02f39237b2d961ade76d403869939004965eff53d529d436b117c3a") )
        ( 900000,   uint256("0x79ae3d0df2e1fc905781ce893d3ead49799f8100c606ae613389dd3f7c1161f2") )
        ( 1000000,  uint256("0x9e2738c1c40209ee73a9ca0ea1edaa3610d768007b614fb334bdbcb46d2d1991") )
        ( 1100000,  uint256("0x33dfe0abae17bc1cff0a8f3d3f9c4471666d9092182963baa61a2cb3d0ad9283") )
        ( 1200000,  uint256("0x135c94cb8cdf81173ceb6d01f98df1b74ab0bf95aa3553a2158fef3bc733794d") )
        ( 1232433,  uint256("0x5a17e0f06d3863b5678b5f24b4b1e203dc0d94ec55b4f41f5d9d2a961686b0c2") )
        ( 1237500,  uint256("0xa3e0855b308dde2573da3be0a4761300e09842250303c1c30e519f9a7d2a7d4b") )
        ;

    // TestNet has no checkpoints
    MapCheckpoints mapCheckpointsTestnet;

    bool CheckHardened(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end())
            return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        if (checkpoints.empty())
            return 0;
        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

    CBlockThinIndex* GetLastCheckpoint(const std::map<uint256, CBlockThinIndex*>& mapBlockThinIndex)
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockThinIndex*>::const_iterator t = mapBlockThinIndex.find(hash);
            if (t != mapBlockThinIndex.end())
                return t->second;
        }
        return NULL;
    }


    // Automatically select a suitable sync-checkpoint
    const CBlockIndex* AutoSelectSyncCheckpoint()
    {
        const CBlockIndex *pindex = pindexBest;
        // Search backward for a block within max span and maturity window
        while (pindex->pprev && pindex->nHeight + nCheckpointSpan > pindexBest->nHeight)
            pindex = pindex->pprev;
        return pindex;
    }

    // Automatically select a suitable sync-checkpoint - Thin mode
    const CBlockThinIndex* AutoSelectSyncThinCheckpoint()
    {
        const CBlockThinIndex *pindex = pindexBestHeader;
        // Search backward for a block within max span and maturity window
        while (pindex->pprev && pindex->nHeight + nCheckpointSpan > pindexBest->nHeight)
            pindex = pindex->pprev;
        return pindex;
    }

    // Check against synchronized checkpoint
    bool CheckSync(int nHeight)
    {
        if(nNodeMode == NT_FULL)
        {
            const CBlockIndex* pindexSync = AutoSelectSyncCheckpoint();

            if (nHeight <= pindexSync->nHeight)
                return false;
        }
        else {
            const CBlockThinIndex *pindexSync = AutoSelectSyncThinCheckpoint();

            if (nHeight <= pindexSync->nHeight)
                return false;
        }
        return true;
    }
}
