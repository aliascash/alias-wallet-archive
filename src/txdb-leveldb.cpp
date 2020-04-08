// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <map>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <leveldb/env.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include <memenv/memenv.h>

#include "kernel.h"
#include "checkpoints.h"
#include "txdb.h"
#include "util.h"
#include "main.h"

using namespace std;
namespace fs = boost::filesystem;

leveldb::DB *txdb; // global pointer for LevelDB object instance

static leveldb::Options GetOpenOptions() {
    leveldb::Options options;
    int nCacheSizeMB = GetArg("-dbcache", 25);
    options.create_if_missing = true;
    options.block_cache = leveldb::NewLRUCache(nCacheSizeMB * 1048576);
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    return options;
}

static void init_blockindex(leveldb::Options& options, bool fRemoveOld = false)
{
    // First time init.
    fs::path directory = GetDataDir() / "txleveldb";

    if (fRemoveOld)
        fs::remove_all(directory);

    LogPrintf("Opening LevelDB in %s\n", directory.string().c_str());
    leveldb::Status status = leveldb::DB::Open(options, directory.string(), &txdb);
    if (!status.ok()) {
        throw runtime_error(strprintf("init_blockindex(): error opening database environment %s", status.ToString()));
    }
}

// CDB subclasses are created and destroyed VERY OFTEN. That's why
// we shouldn't treat this as a free operations.
CTxDB::CTxDB(const char* pszMode)
{
    assert(pszMode);
    activeBatch = NULL;
    fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));

    if (txdb) {
        pdb = txdb;
        return;
    }

    openOptions = GetOpenOptions();
    init_blockindex(openOptions);
    pdb = txdb;

    LogPrintf("Opened LevelDB successfully\n");
}

void CTxDB::Close()
{
    delete txdb;
    txdb = pdb = NULL;

    if (activeBatch)
    {
        delete activeBatch;
        activeBatch = NULL;
    }

    if (openOptions.block_cache)
    {
        delete openOptions.block_cache;
        openOptions.block_cache = NULL;
    }

    if (openOptions.filter_policy)
    {
        openOptions.filter_policy = NULL;
    }

}

bool CTxDB::TxnBegin()
{
    assert(!activeBatch);
    activeBatch = new leveldb::WriteBatch();
    return true;
}

bool CTxDB::TxnCommit()
{
    assert(activeBatch);
    leveldb::Status status = pdb->Write(GetWriteOptions(), activeBatch);
    delete activeBatch;
    activeBatch = NULL;
    if (!status.ok()) {
        LogPrintf("LevelDB batch commit failure: %s\n", status.ToString());
        return false;
    }
    return true;
}

class CBatchScanner : public leveldb::WriteBatch::Handler {
public:
    std::string needle;
    bool *deleted;
    std::string *foundValue;
    bool foundEntry;

    CBatchScanner() : foundEntry(false) {}

    virtual void Put(const leveldb::Slice& key, const leveldb::Slice& value) {
        if (key.ToString() == needle) {
            foundEntry = true;
            *deleted = false;
            *foundValue = value.ToString();
        }
    }

    virtual void Delete(const leveldb::Slice& key) {
        if (key.ToString() == needle) {
            foundEntry = true;
            *deleted = true;
        }
    }
};

// When performing a read, if we have an active batch we need to check it first
// before reading from the database, as the rest of the code assumes that once
// a database transaction begins reads are consistent with it. It would be good
// to change that assumption in future and avoid the performance hit, though in
// practice it does not appear to be large.
bool CTxDB::ScanBatch(const CDataStream &key, string *value, bool *deleted) const {
    assert(activeBatch);
    *deleted = false;
    CBatchScanner scanner;
    scanner.needle = key.str();
    scanner.deleted = deleted;
    scanner.foundValue = value;
    leveldb::Status status = activeBatch->Iterate(&scanner);
    if (!status.ok()) {
        throw runtime_error(status.ToString());
    }
    return scanner.foundEntry;
}

int CTxDB::CheckVersion()
{
    if (Exists(string("version")))
    {
        ReadVersion(nVersion);
        LogPrintf("Transaction index version is %d\n", nVersion);

        if (nVersion < DATABASE_VERSION)
        {
            LogPrintf("Required index version is %d.\n", DATABASE_VERSION);

            RecreateDB();

            return 2;
        };
    } else
    {
        bool fTmp = fReadOnly;
        fReadOnly = false;
        WriteVersion(DATABASE_VERSION);
        fReadOnly = fTmp;
    };
    return 0;
};

int CTxDB::RecreateDB()
{
    LogPrintf("Recreating TXDB.\n");

    delete txdb;
    txdb = pdb = NULL;

    if (activeBatch)
    {
        delete activeBatch;
        activeBatch = NULL;
    }

    if (openOptions.block_cache) {
        delete openOptions.block_cache;
        openOptions.block_cache = NULL;
    }

    if (openOptions.filter_policy) {
        openOptions.filter_policy = NULL;
    }

    openOptions = GetOpenOptions();
    init_blockindex(openOptions, true); // Remove directory and create new database
    pdb = txdb;

    bool fTmp = fReadOnly;
    fReadOnly = false;
    WriteVersion(DATABASE_VERSION);
    fReadOnly = fTmp;

    return 0;
};

bool CTxDB::WriteKeyImage(ec_point& keyImage, CKeyImageSpent& keyImageSpent)
{
    return Write(make_pair(string("ki"), keyImage), keyImageSpent);
};

bool CTxDB::ReadKeyImage(ec_point& keyImage, CKeyImageSpent& keyImageSpent)
{
    return Read(make_pair(string("ki"), keyImage), keyImageSpent);
};

bool CTxDB::EraseKeyImage(ec_point& keyImage)
{
    return Erase(make_pair(string("ki"), keyImage));
}

bool CTxDB::WriteAnonOutput(CPubKey& pkCoin, CAnonOutput& ao)
{
    return Write(make_pair(string("ao"), pkCoin), ao);
};

bool CTxDB::ReadAnonOutput(CPubKey& pkCoin, CAnonOutput& ao)
{
    return Read(make_pair(string("ao"), pkCoin), ao);
};

bool CTxDB::EraseAnonOutput(CPubKey& pkCoin)
{
    return Erase(make_pair(string("ao"), pkCoin));
};

bool CTxDB::WriteCompromisedAnonHeights(std::map<int64_t, std::vector<int>>& mapCompromisedHeights)
{
    return Write(string("compromisedanonheights"), mapCompromisedHeights);
};

bool CTxDB::ReadCompromisedAnonHeights(std::map<int64_t, std::vector<int>>& mapCompromisedHeights)
{
    return Read(string("compromisedanonheights"), mapCompromisedHeights);
};

bool CTxDB::EraseCompromisedAnonHeights()
{
    return Erase(string("compromisedanonheights"));
};

bool CTxDB::EraseRange(const std::string &sPrefix, uint32_t &nAffected, std::function<void (const uint32_t&)> funcProgress)
{

    TxnBegin();

    leveldb::Iterator *iterator = pdb->NewIterator(GetReadOptions());
    if (!iterator)
        LogPrintf("EraseRange(%s) - NewIterator failed.\n", sPrefix.c_str());

    size_t nLenPrefix = sPrefix.length();

    if (nLenPrefix > 252) // fit in 256 and compressed int is 1 byte
    {
        LogPrintf("EraseRange(%s) - Key length too long.\n", sPrefix.c_str());
        return false;
    };

    // - key starts with strlen || str
    uint8_t data[256];
    data[0] = (uint8_t)nLenPrefix;
    memcpy(&data[1], sPrefix.data(), nLenPrefix);

    iterator->Seek(leveldb::Slice((const char*)data, nLenPrefix+1));

    leveldb::WriteOptions writeOptions = GetWriteOptions();
    while (iterator->Valid())
    {
        if (iterator->key().size() < nLenPrefix+1
            || memcmp(iterator->key().data(), data, nLenPrefix+1) != 0)
            break;

        leveldb::Status s = pdb->Delete(writeOptions, iterator->key());

        if (!s.ok())
            LogPrintf("EraseRange(%s) - Delete failed.\n", sPrefix.c_str());

        if (funcProgress && nAffected % 100 == 0) funcProgress(nAffected);

        nAffected++;
        iterator->Next();
    };


    delete iterator;
    TxnCommit();

    return true;
};


bool CTxDB::ReadTxIndex(uint256 hash, CTxIndex& txindex)
{
    txindex.SetNull();
    return Read(make_pair(string("tx"), hash), txindex);
}

bool CTxDB::UpdateTxIndex(uint256 hash, const CTxIndex& txindex)
{
    return Write(make_pair(string("tx"), hash), txindex);
}

bool CTxDB::AddTxIndex(const CTransaction& tx, const CDiskTxPos& pos, int nHeight)
{
    // Add to tx index
    uint256 hash = tx.GetHash();
    CTxIndex txindex(pos, tx.vout.size());
    return Write(make_pair(string("tx"), hash), txindex);
}

bool CTxDB::EraseTxIndex(const CTransaction& tx)
{
    uint256 hash = tx.GetHash();

    return Erase(make_pair(string("tx"), hash));
}

bool CTxDB::ContainsTx(uint256 hash)
{
    return Exists(make_pair(string("tx"), hash));
}

bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx, CTxIndex& txindex)
{
    tx.SetNull();
    if (!ReadTxIndex(hash, txindex))
        return false;
    return (tx.ReadFromDisk(txindex.pos));
}

bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx)
{
    CTxIndex txindex;
    return ReadDiskTx(hash, tx, txindex);
}

bool CTxDB::ReadDiskTx(COutPoint outpoint, CTransaction& tx, CTxIndex& txindex)
{
    return ReadDiskTx(outpoint.hash, tx, txindex);
}

bool CTxDB::ReadDiskTx(COutPoint outpoint, CTransaction& tx)
{
    CTxIndex txindex;
    return ReadDiskTx(outpoint.hash, tx, txindex);
}

bool CTxDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(make_pair(string("bidx"), blockindex.GetBlockHash()), blockindex);
}

bool CTxDB::EraseBlockIndex(const uint256& blockhash)
{
    return Erase(make_pair(string("bidx"), blockhash));
}

bool CTxDB::WriteBlockThinIndex(const CDiskBlockThinIndex& blockindex)
{
    return Write(make_pair(string("bhidx"), blockindex.GetBlockHash()), blockindex);
}

bool CTxDB::ReadBlockThinIndex(const uint256& hash, CDiskBlockThinIndex& blockindex)
{
    return Read(make_pair(string("bhidx"), hash), blockindex);
};

bool CTxDB::ReadHashBestChain(uint256& hashBestChain)
{
    return Read(string("hashBestChain"), hashBestChain);
}

bool CTxDB::WriteHashBestChain(uint256 hashBestChain)
{
    return Write(string("hashBestChain"), hashBestChain);
}

bool CTxDB::ReadHashBestHeaderChain(uint256& hashBestChain)
{
    return Read(string("hashBestHeaderChain"), hashBestChain);
};

bool CTxDB::WriteHashBestHeaderChain(uint256 hashBestChain)
{
    return Write(string("hashBestHeaderChain"), hashBestChain);
};


bool CTxDB::ReadBestInvalidTrust(CBigNum& bnBestInvalidTrust)
{
    return Read(string("bnBestInvalidTrust"), bnBestInvalidTrust);
}

bool CTxDB::WriteBestInvalidTrust(CBigNum bnBestInvalidTrust)
{
    return Write(string("bnBestInvalidTrust"), bnBestInvalidTrust);
}

static CBlockIndex *InsertBlockIndex(uint256 hash)
{
    if (hash == 0)
        return NULL;

    // Return existing
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    // Create new
    CBlockIndex* pindexNew = new CBlockIndex();
    if (!pindexNew)
        throw runtime_error("LoadBlockIndex() : new CBlockIndex failed");
    mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}

bool CTxDB::LoadBlockIndex(std::function<bool (const CBlockIndex* const)> funcValidate, std::function<void (const uint32_t&)> funcProgress)
{
    if (nNodeMode != NT_FULL)
        return 0;

    if (mapBlockIndex.size() > 0)
    {
        // Already loaded once in this session. It can happen during migration
        // from BDB.
        return true;
    };


    // The block index is an in-memory structure that maps hashes to on-disk
    // locations where the contents of the block can be found. Here, we scan it
    // out of the DB and into mapBlockIndex.
    leveldb::Iterator *iterator = pdb->NewIterator(GetReadOptions());
    // Seek to start key.
    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("bidx"), uint256(0));
    iterator->Seek(ssStartKey.str());

    uint32_t count = 0;
    // Now read each entry.
    while (iterator->Valid())
    {
        if (funcProgress && count != 0 && count % 10000 == 0) funcProgress(count);
        count++;
        boost::this_thread::interruption_point();
        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());
        string strType;
        ssKey >> strType;
        // Did we reach the end of the data to read?
        if (strType != "bidx")
            break;

        uint256 blockHash;
        ssKey >> blockHash;

        CDiskBlockIndex diskindex;
        ssValue >> diskindex;


        // Construct block index object
        CBlockIndex* pindexNew       = InsertBlockIndex(blockHash);
        pindexNew->pprev             = InsertBlockIndex(diskindex.hashPrev);
        pindexNew->pnext             = InsertBlockIndex(diskindex.hashNext);
        pindexNew->nFile             = diskindex.nFile;
        pindexNew->nBlockPos         = diskindex.nBlockPos;
        pindexNew->nHeight           = diskindex.nHeight;
        pindexNew->nMint             = diskindex.nMint;
        pindexNew->nMoneySupply      = diskindex.nMoneySupply;
        pindexNew->nAnonSupply       = diskindex.nAnonSupply;
        pindexNew->nFlags            = diskindex.nFlags;
        pindexNew->nStakeModifier    = diskindex.nStakeModifier;
        pindexNew->bnStakeModifierV2 = diskindex.bnStakeModifierV2;
        pindexNew->prevoutStake      = diskindex.prevoutStake;
        pindexNew->nStakeTime        = diskindex.nStakeTime;
        pindexNew->hashProof         = diskindex.hashProof;
        pindexNew->nVersion          = diskindex.nVersion;
        pindexNew->hashMerkleRoot    = diskindex.hashMerkleRoot;
        pindexNew->nTime             = diskindex.nTime;
        pindexNew->nBits             = diskindex.nBits;
        pindexNew->nNonce            = diskindex.nNonce;

        // Watch for genesis block
        if (pindexGenesisBlock == NULL && blockHash == Params().HashGenesisBlock())
            pindexGenesisBlock = pindexNew;

        if (!pindexNew->CheckIndex()) {
            delete iterator;
            return error("LoadBlockIndex() : CheckIndex failed at %d", pindexNew->nHeight);
        }

        if (funcValidate && !funcValidate(pindexNew))
            return false;

        // NovaCoin: build setStakeSeen
        if (pindexNew->IsProofOfStake())
            setStakeSeen.insert(make_pair(pindexNew->prevoutStake, pindexNew->nStakeTime));

        iterator->Next();
    }
    delete iterator;
    if (funcProgress) funcProgress(count);

    boost::this_thread::interruption_point();

    // Load hashBestChain pointer to end of best chain
    if (!ReadHashBestChain(hashBestChain))
    {
        if (pindexGenesisBlock == NULL)
            return true;
        return error("CTxDB::LoadBlockIndex() : hashBestChain not loaded");
    }
    if (!mapBlockIndex.count(hashBestChain))
        return error("CTxDB::LoadBlockIndex() : hashBestChain not found in the block index");

    pindexBest = mapBlockIndex[hashBestChain];
    nBestHeight = pindexBest->nHeight;
    chainActive.SetTip(pindexBest);

    // Calculate nChainTrust
    vector<pair<int, CBlockIndex*> > vSortedByHeight;
    vSortedByHeight.reserve(mapBlockIndex.size());
    BOOST_FOREACH(const PAIRTYPE(uint256, CBlockIndex*)& item, mapBlockIndex)
    {
        CBlockIndex* pindex = item.second;
        vSortedByHeight.push_back(make_pair(pindex->nHeight, pindex));
    }
    sort(vSortedByHeight.begin(), vSortedByHeight.end());
    BOOST_FOREACH(const PAIRTYPE(int, CBlockIndex*)& item, vSortedByHeight)
    {
        CBlockIndex* pindex = item.second;

        uint256 blockhash = pindex->GetBlockHash();
        if ((!pindex->pprev && blockhash != Params().HashGenesisBlock()) || pindex->nHeight > nBestHeight)
        {
            mapBlockIndex.erase(blockhash);
            if (fDebug)
                LogPrintf("LoadBlockIndex(): Warning - Found orphaned block, height %d, hash %s. Suggest rewindchain, reindex.\n", pindex->nHeight, pindex->GetBlockHash().ToString().c_str());
            if (pindex->nHeight > nBestHeight)
            {
                CBlock block;
                if (block.ReadFromDisk(pindex))
                    AddOrphanBlock(&block);
            }
            continue;
        };

        pindex->nChainTrust = (pindex->pprev ? pindex->pprev->nChainTrust : 0) + pindex->GetBlockTrust();
    }

    nBestChainTrust = pindexBest->nChainTrust;

    LogPrintf("LoadBlockIndex(): hashBestChain=%s  height=%d  trust=%s  date=%s\n",
      hashBestChain.ToString(), nBestHeight, CBigNum(nBestChainTrust).ToString(),
      DateTimeStrFormat("%x %H:%M:%S", pindexBest->GetBlockTime()));

    // Load bnBestInvalidTrust, OK if it doesn't exist
    CBigNum bnBestInvalidTrust;
    ReadBestInvalidTrust(bnBestInvalidTrust);
    nBestInvalidTrust = bnBestInvalidTrust.getuint256();

    // Verify blocks in the best chain
    int nCheckLevel = GetArg("-checklevel", 1);
    int nCheckDepth = GetArg("-checkblocks", 2500);
    if (nCheckDepth == 0)
        nCheckDepth = 1000000000; // suffices until the year 19000
    if (nCheckDepth > nBestHeight)
        nCheckDepth = nBestHeight;
    LogPrintf("Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    CBlockIndex* pindexFork = NULL;
    map<pair<unsigned int, unsigned int>, CBlockIndex*> mapBlockPos;
    for (CBlockIndex* pindex = pindexBest; pindex && pindex->pprev; pindex = pindex->pprev)
    {
        boost::this_thread::interruption_point();
        if (pindex->nHeight < nBestHeight-nCheckDepth)
            break;
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("LoadBlockIndex() : block.ReadFromDisk failed");
        // check level 1: verify block validity
        // check level 7: verify block signature too
        if (nCheckLevel>0 && !block.CheckBlock(true, true, (nCheckLevel>6)))
        {
            LogPrintf("LoadBlockIndex() : *** found bad block at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            pindexFork = pindex->pprev;
        }
        // check level 2: verify transaction index validity
        if (nCheckLevel>1)
        {
            pair<unsigned int, unsigned int> pos = make_pair(pindex->nFile, pindex->nBlockPos);
            mapBlockPos[pos] = pindex;
            BOOST_FOREACH(const CTransaction &tx, block.vtx)
            {
                uint256 hashTx = tx.GetHash();
                CTxIndex txindex;
                if (ReadTxIndex(hashTx, txindex))
                {
                    // check level 3: checker transaction hashes
                    if (nCheckLevel>2 || pindex->nFile != txindex.pos.nFile || pindex->nBlockPos != txindex.pos.nBlockPos)
                    {
                        // either an error or a duplicate transaction
                        CTransaction txFound;
                        if (!txFound.ReadFromDisk(txindex.pos))
                        {
                            LogPrintf("LoadBlockIndex() : *** cannot read mislocated transaction %s\n", hashTx.ToString());
                            pindexFork = pindex->pprev;
                        }
                        else
                            if (txFound.GetHash() != hashTx) // not a duplicate tx
                            {
                                LogPrintf("LoadBlockIndex(): *** invalid tx position for %s\n", hashTx.ToString());
                                pindexFork = pindex->pprev;
                            }
                    }
                    // check level 4: check whether spent txouts were spent within the main chain
                    unsigned int nOutput = 0;
                    if (nCheckLevel>3)
                    {
                        BOOST_FOREACH(const CDiskTxPos &txpos, txindex.vSpent)
                        {
                            if (!txpos.IsNull())
                            {
                                pair<unsigned int, unsigned int> posFind = make_pair(txpos.nFile, txpos.nBlockPos);
                                if (!mapBlockPos.count(posFind))
                                {
                                    LogPrintf("LoadBlockIndex(): *** found bad spend at %d, hashBlock=%s, hashTx=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString(), hashTx.ToString());
                                    pindexFork = pindex->pprev;
                                }
                                // check level 6: check whether spent txouts were spent by a valid transaction that consume them
                                if (nCheckLevel>5)
                                {
                                    CTransaction txSpend;
                                    if (!txSpend.ReadFromDisk(txpos))
                                    {
                                        LogPrintf("LoadBlockIndex(): *** cannot read spending transaction of %s:%i from disk\n", hashTx.ToString(), nOutput);
                                        pindexFork = pindex->pprev;
                                    }
                                    else if (!txSpend.CheckTransaction())
                                    {
                                        LogPrintf("LoadBlockIndex(): *** spending transaction of %s:%i is invalid\n", hashTx.ToString(), nOutput);
                                        pindexFork = pindex->pprev;
                                    }
                                    else
                                    {
                                        bool fFound = false;
                                        BOOST_FOREACH(const CTxIn &txin, txSpend.vin)
                                            if (txin.prevout.hash == hashTx && txin.prevout.n == nOutput)
                                                fFound = true;
                                        if (!fFound)
                                        {
                                            LogPrintf("LoadBlockIndex(): *** spending transaction of %s:%i does not spend it\n", hashTx.ToString(), nOutput);
                                            pindexFork = pindex->pprev;
                                        }
                                    }
                                }
                            }
                            nOutput++;
                        }
                    }
                }
                // check level 5: check whether all prevouts are marked spent
                if (nCheckLevel>4)
                {
                     BOOST_FOREACH(const CTxIn &txin, tx.vin)
                     {
                          CTxIndex txindex;
                          if (ReadTxIndex(txin.prevout.hash, txindex))
                              if (txindex.vSpent.size()-1 < txin.prevout.n || txindex.vSpent[txin.prevout.n].IsNull())
                              {
                                  LogPrintf("LoadBlockIndex(): *** found unspent prevout %s:%i in %s\n", txin.prevout.hash.ToString(), txin.prevout.n, hashTx.ToString());
                                  pindexFork = pindex->pprev;
                              }
                     }
                }
            }
        }
    }
    if (pindexFork)
    {
        boost::this_thread::interruption_point();
        // Reorg back to the fork
        LogPrintf("LoadBlockIndex() : *** moving best chain pointer back to block %d\n", pindexFork->nHeight);
        CBlock block;
        if (!block.ReadFromDisk(pindexFork))
            return error("LoadBlockIndex() : block.ReadFromDisk failed");
        CTxDB txdb;
        block.SetBestChain(txdb, pindexFork);
    }

    return true;
}

bool CTxDB::LoadBlockThinIndex()
{
    if (fDebug)
        LogPrintf("CTxDB::LoadBlockThinIndex()\n");

    if (mapBlockThinIndex.size() > 0)
    {
        // Already loaded
        return true;
    };



    // TODO: allocate once is possible as using hard limit on items in map

    uint256 hashNext = Params().HashGenesisBlock();

    CDiskBlockThinIndex diskindex;
    map<uint256, CBlockThinIndex*>::iterator mi;
    CBlockThinIndex* pIndexLast = NULL;

    while (hashNext != 0)
    {
        if (!ReadBlockThinIndex(hashNext, diskindex))
        {
            LogPrintf("LoadBlockThinIndex() Read header %s failed.\n", hashNext.ToString().c_str());
            break;
        };

        //LogPrintf("[rem] bhidx %s\n", hashNext.ToString().c_str());

        // Construct block index object
        CBlockThinIndex* pindexNew      = new CBlockThinIndex();
        if (!pindexNew)
            return error("LoadBlockThinIndex() : new CBlockIndex failed");

        mi = mapBlockThinIndex.insert(make_pair(hashNext, pindexNew)).first;
        pindexNew->phashBlock = &(mi->first);


        pindexNew->nFile                = diskindex.nFile;
        pindexNew->nBlockPos            = diskindex.nBlockPos;
        pindexNew->nHeight              = diskindex.nHeight;
        pindexNew->nFlags               = diskindex.nFlags;
        pindexNew->nStakeModifier       = diskindex.nStakeModifier;
        pindexNew->hashProof            = diskindex.hashProof;
        pindexNew->nVersion             = diskindex.nVersion;
        pindexNew->hashMerkleRoot       = diskindex.hashMerkleRoot;
        pindexNew->nTime                = diskindex.nTime;
        pindexNew->nBits                = diskindex.nBits;
        pindexNew->nNonce               = diskindex.nNonce;

        pindexNew->pprev                = pIndexLast;
        if (pIndexLast)
            pIndexLast->pnext           = pindexNew;

        pindexNew->nChainTrust = (pIndexLast ? pIndexLast->nChainTrust : 0) + pindexNew->GetBlockTrust();


        // -- genesis block will always be first
        if (pindexGenesisBlockThin == NULL)
        {
            pindexGenesisBlockThin = pindexNew;
            pindexRear = pindexGenesisBlockThin;
        };



        // pindexNew->CheckIndex() does nothing !?

        pIndexLast = pindexNew;
        hashNext = diskindex.hashNext;


        while (!fThinFullIndex && pindexRear
            && pindexNew->nHeight - pindexRear->nHeight > nThinIndexWindow)
        {
            const uint256* pRemHash = pindexRear->phashBlock;

            pindexRear = pindexRear->pnext;
            pindexRear->pprev = NULL;

            std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(*pRemHash);


            if (mi != mapBlockThinIndex.end())
            {
                delete mi->second;
                mapBlockThinIndex.erase(mi);
            };
        };


    };

    // -- load hashBestChain pointer to end of best chain
    if (!ReadHashBestHeaderChain(hashBestChain))
    {
        if (pindexGenesisBlockThin == NULL)
            return true;
        return error("CTxDB::LoadBlockThinIndex() : hashBestChain not loaded");
    };

    if (!mapBlockThinIndex.count(hashBestChain))
        return error("CTxDB::LoadBlockThinIndex() : hashBestChain not found in the block index");

    pindexBestHeader = mapBlockThinIndex[hashBestChain];

    nBestHeight = pindexBestHeader->nHeight;
    nBestChainTrust = pindexBestHeader->nChainTrust;

    LogPrintf("LoadBlockThinIndex(): hashBestChain=%s  height=%d  trust=%s  date=%s\n",
        hashBestChain.ToString().substr(0,20).c_str(), nBestHeight, CBigNum(nBestChainTrust).ToString().c_str(),
        DateTimeStrFormat("%x %H:%M:%S", pindexBestHeader->GetBlockTime()).c_str());


    return true;
}

