// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
// SPDX-FileCopyrightText: © 2014 BlackCoin Developers
// SPDX-FileCopyrightText: © 2013 NovaCoin Developers
// SPDX-FileCopyrightText: © 2011 PPCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "txdb.h"
#include "wallet.h"
#include "walletdb.h"
#include "bloom.h"
#include "crypter.h"
#include "interface.h"
#include "base58.h"
#include "kernel.h"
#include "coincontrol.h"
#include "pbkdf2.h"
#include <chrono>
#include <random>
#include <boost/algorithm/string/replace.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>


using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// mapWallet
//

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t1,
                    const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

CPubKey CWallet::GenerateNewKey()
{
    //assert(false); // [rm] replace with HD - needed for NewKeyPool from EncryptWallet
    //LogPrintf("[rm] GenerateNewKey()\n");

    AssertLockHeld(cs_wallet); // mapKeyMetadata
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    RandAddSeedPerfmon();
    CKey secret;
    secret.MakeNewKey(fCompressed);

    // Compressed public keys were introduced in version 0.6.0
    if (fCompressed)
        SetMinVersion(FEATURE_COMPRPUBKEY);

    CPubKey pubkey = secret.GetPubKey();

    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewKey() : AddKeyPubKey failed");
    return pubkey;
}

bool CWallet::AddKey(const CKey &secret)
{
    return CWallet::AddKeyPubKey(secret, secret.GetPubKey());
}

bool CWallet::AddKeyPubKey(const CKey &secret, const CPubKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    if (!fFileBacked)
        return true;
    if (!IsCrypted()) {
        return CWalletDB(strWalletFile).WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
    }
    return true;
}

int CWallet::Finalise()
{
    SetBestChain(CBlockLocator(pindexBest));

    ExtKeyAccountMap::iterator it = mapExtAccounts.begin();
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
        if (it->second)
            delete it->second;

    ExtKeyMap::iterator itl = mapExtKeys.begin();
    for (itl = mapExtKeys.begin(); itl != mapExtKeys.end(); ++itl)
        if (itl->second)
            delete itl->second;

    if (pBloomFilter)
    {
        delete pBloomFilter;
        if (fDebug)
            LogPrintf("Bloom filter destructed.\n");
    };

    return 0;
};

bool CWallet::AddKeyInDBTxn(CWalletDB *pdb, const CKey &key)
{
    LOCK(cs_KeyStore);
    // -- can't use CWallet::AddKey(), as in a db transaction
    //    hack: pwalletdbEncryption CCryptoKeyStore::AddKey calls CWallet::AddCryptedKey
    //    DB Write() uses activeTxn
    CWalletDB *pwalletdbEncryptionOld = pwalletdbEncryption;
    pwalletdbEncryption = pdb;

    CPubKey pubkey = key.GetPubKey();
    bool rv = CCryptoKeyStore::AddKeyPubKey(key, pubkey);

    pwalletdbEncryption = pwalletdbEncryptionOld;

    if (!rv)
    {
        LogPrintf("CCryptoKeyStore::AddKeyPubKey failed.\n");
        return false;
    };

    if (fFileBacked
        && !IsCrypted())
    {
        if (!pdb->WriteKey(pubkey, key.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]))
        {
            LogPrintf("WriteKey() failed.\n");
            return false;
        };
    };
    return true;
};

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey, const vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;

    if (!fFileBacked)
        return true;

    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
    }

    return false;
}

bool CWallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(redeemScript))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteCScript(Hash160(redeemScript), redeemScript);
}

// optional setting to unlock wallet for staking only
// serves to disable the trivial sendmoney when OS account compromised
// provides no real security
bool fWalletUnlockStakingOnly = false;

bool CWallet::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = CBitcoinAddress(redeemScript.GetID()).ToString();
        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %u which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n",
            __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr.c_str());
        return true;
    };

    return CCryptoKeyStore::AddCScript(redeemScript);
}

bool CWallet::Lock()
{
    if (fDebug)
        LogPrintf("CWallet::Lock()\n");

    if (IsLocked())
        return true;

    LogPrintf("Locking wallet.\n");

    {
        LOCK(cs_wallet);
        CWalletDB wdb(strWalletFile);

        // -- load encrypted spend_secret of stealth addresses
        CStealthAddress sxAddrTemp;
        std::set<CStealthAddress>::iterator it;
        for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
        {
            if (it->scan_secret.size() < 32)
                continue; // stealth address is not owned
            // -- CStealthAddress are only sorted on spend_pubkey
            CStealthAddress &sxAddr = const_cast<CStealthAddress&>(*it);
            if (fDebug)
                LogPrintf("Recrypting stealth key %s\n", sxAddr.Encoded().c_str());

            sxAddrTemp.scan_pubkey = sxAddr.scan_pubkey;
            if (!wdb.ReadStealthAddress(sxAddrTemp))
            {
                LogPrintf("Error: Failed to read stealth key from db %s\n", sxAddr.Encoded().c_str());
                continue;
            }
            sxAddr.spend_secret = sxAddrTemp.spend_secret;
        };
        ExtKeyLock();
    }
    return LockKeyStore();
};

bool CWallet::Unlock(const SecureString& strWalletPassphrase)
{
    if (fDebug)
        LogPrintf("CWallet::Unlock()\n");

    if (!IsLocked()
        || !IsCrypted())
        return false;

    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    LogPrintf("Unlocking wallet.\n");

    {
        LOCK2(cs_main, cs_wallet);
        BOOST_FOREACH(const MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (!CCryptoKeyStore::Unlock(vMasterKey))
                return false;
            break;
        };

        UnlockStealthAddresses(vMasterKey);
        ExtKeyUnlock(vMasterKey);
        ProcessLockedAnonOutputs();

        if (fMakeExtKeyInitials)
        {
            fMakeExtKeyInitials = false;
            CWalletDB wdb(strWalletFile, "r+");
            if (ExtKeyCreateInitial(&wdb, GetArg("-bip44key", "")) != 0)
            {
               LogPrintf("Warning: ExtKeyCreateInitial failed.\n");
            };
        };

    } // cs_main, cs_wallet

    return true;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        BOOST_FOREACH(MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if (!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey)
                && UnlockStealthAddresses(vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                LogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;

                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
                return true;
            };
        };
    } // cs_wallet

    return false;
}

void CWallet::SetBestChain(const CBlockLocator& loc)
{
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlock(loc);
}

void CWallet::SetBestThinChain(const CBlockThinLocator& loc)
{
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlockThin(loc);
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn, bool fExplicit)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
        nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    if (fFileBacked)
    {
        CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        if (nWalletVersion > 40000)
            pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn)
            delete pwalletdb;
    };

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{
    LOCK(cs_wallet); // nWalletVersion, nWalletMaxVersion
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial vMasterKey;
    RandAddSeedPerfmon();

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    RAND_bytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey(nDerivationMethodIndex);

    RandAddSeedPerfmon();
    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    RAND_bytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK2(cs_main, cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked)
        {
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin())
                return false;
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        };

        if (!EncryptKeys(vMasterKey))
        {
            if (fFileBacked)
                pwalletdbEncryption->TxnAbort();
            exit(1); //We now probably have half of our keys encrypted in memory, and half not...die and let the user reload their unencrypted wallet.
        };

        std::set<CStealthAddress>::iterator it;
        for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
        {
            if (it->scan_secret.size() < 32)
                continue; // stealth address is not owned
            // -- CStealthAddress is only sorted on spend_pubkey
            CStealthAddress &sxAddr = const_cast<CStealthAddress&>(*it);

            if (fDebug)
                LogPrintf("Encrypting stealth key %s\n", sxAddr.Encoded().c_str());

            std::vector<unsigned char> vchCryptedSecret;

            CSecret vchSecret;
            vchSecret.resize(32);
            memcpy(&vchSecret[0], &sxAddr.spend_secret[0], 32);

            uint256 iv = Hash(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end());
            if (!EncryptSecret(vMasterKey, vchSecret, iv, vchCryptedSecret))
            {
                LogPrintf("Error: Failed encrypting stealth key %s\n", sxAddr.Encoded().c_str());
                continue;
            };

            sxAddr.spend_secret = vchCryptedSecret;
            pwalletdbEncryption->WriteStealthAddress(sxAddr);
        };

        if (0 != ExtKeyEncryptAll(pwalletdbEncryption, vMasterKey))
        {
            LogPrintf("Terminating - Error: ExtKeyEncryptAll failed.\n");
            exit(1); // wallet on disk is still uncrypted
        };

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption, true);

        if (fFileBacked)
        {
            if (!pwalletdbEncryption->TxnCommit())
                exit(1); //We now have keys encrypted in memory, but no on disk...die to avoid confusion and let the user reload their unencrypted wallet.

            delete pwalletdbEncryption;
            pwalletdbEncryption = NULL;
        };

        Lock();
        Unlock(strWalletPassphrase);
        NewKeyPool();
        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);

    }
    NotifyStatusChanged(this);

    return true;
}

int64_t CWallet::IncOrderPosNext(CWalletDB *pwalletdb)
{
    AssertLockHeld(cs_wallet); // nOrderPosNext
    int64_t nRet = nOrderPosNext++;
    if (pwalletdb)
    {
        pwalletdb->WriteOrderPosNext(nOrderPosNext);
    } else
    {
        CWalletDB(strWalletFile).WriteOrderPosNext(nOrderPosNext);
    };

    return nRet;
}

CWallet::TxItems CWallet::OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount, bool fShowCoinstake)
{
    AssertLockHeld(cs_wallet); // mapWallet
    CWalletDB walletdb(strWalletFile);

    // First: get all CWalletTx and CAccountingEntry into a sorted-by-order multimap.
    TxItems txOrdered;

    // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
    // would make this much faster for applications that do this a lot.
    for (WalletTxMap::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx* wtx = &((*it).second);

        if (!fShowCoinstake
            && wtx->IsCoinStake())
            continue;

        txOrdered.insert(make_pair(wtx->nOrderPos, TxPair(wtx, (CAccountingEntry*)0)));
    };

    acentries.clear();
    walletdb.ListAccountCreditDebit(strAccount, acentries);
    BOOST_FOREACH(CAccountingEntry& entry, acentries)
    {
        txOrdered.insert(make_pair(entry.nOrderPos, TxPair((CWalletTx*)0, &entry)));
    };

    return txOrdered;
}

void CWallet::WalletUpdateSpent(const CTransaction &tx, bool fBlock)
{
    // Anytime a signature is successfully verified, it's proof the outpoint is spent.
    // Update the wallet spent flag if it doesn't know due to wallet.dat being
    // restored from backup or the user making copies of wallet.dat.
    {
        LOCK(cs_wallet);
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            if (tx.nVersion == ANON_TXN_VERSION
                && txin.IsAnonInput())
            {
                // anon input
                // TODO
                continue;
            };

            WalletTxMap::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                CWalletTx& wtx = (*mi).second;
                if (txin.prevout.n >= wtx.vout.size())
                {
                    LogPrintf("WalletUpdateSpent: bad wtx %s\n", wtx.GetHash().ToString().c_str());
                } else
                if (!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                {
                    LogPrintf("WalletUpdateSpent found spent coin %s ALIAS (public) %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkSpent(txin.prevout.n);
                    wtx.WriteToDisk();
                    NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
                };
            };
        };

        if (fBlock)
        {
            uint256 hash = tx.GetHash();
            WalletTxMap::iterator mi = mapWallet.find(hash);
            CWalletTx& wtx = (*mi).second;

            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                if (tx.nVersion == ANON_TXN_VERSION
                    && txout.IsAnonOutput())
                {
                    // anon output
                    // TODO
                    continue;
                };

                if (IsMine(txout))
                {
                    wtx.MarkUnspent(&txout - &tx.vout[0]);
                    wtx.WriteToDisk();
                    NotifyTransactionChanged(this, hash, CT_UPDATED);
                };
            };
        };
    }
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
            item.second.MarkDirty();
    }
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn, const uint256& hashIn)
{
    //uint256 hashIn = wtxIn.GetHash();
    {
        LOCK(cs_wallet);

        // Inserts only if not already there, returns tx inserted or tx found
        pair<WalletTxMap::iterator, bool> ret = mapWallet.insert(make_pair(hashIn, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        wtx.BindWallet(this);
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
        {
            wtx.nTimeReceived = GetAdjustedTime();
            wtx.nOrderPos = IncOrderPosNext();

            wtx.nTimeSmart = wtx.nTimeReceived;
            if (wtxIn.hashBlock != 0)
            {
                bool fInBlockIndex = false;
                unsigned int blocktime;

                if (nNodeMode == NT_FULL)
                {
                    fInBlockIndex = mapBlockIndex.count(wtxIn.hashBlock);
                    blocktime = mapBlockIndex[wtxIn.hashBlock]->nTime;
                } else
                {
                    //fInBlockIndex = mapBlockThinIndex.count(wtxIn.hashBlock);

                    std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtxIn.hashBlock);
                    if (mi == mapBlockThinIndex.end()
                        && !fThinFullIndex
                        && pindexRear)
                    {
                        CTxDB txdb("r");
                        CDiskBlockThinIndex diskindex;
                        if (txdb.ReadBlockThinIndex(wtxIn.hashBlock, diskindex)
                            || diskindex.hashNext != 0)
                        {
                            fInBlockIndex = true;
                            blocktime = diskindex.nTime;
                        };
                    } else
                    {
                        fInBlockIndex = true;
                        blocktime = mi->second->nTime;
                    };
                };

                if (fInBlockIndex)
                {
                    unsigned int latestNow = wtx.nTimeReceived;
                    unsigned int latestEntry = 0;

                    // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
                    int64_t latestTolerated = latestNow + 300;
                    std::list<CAccountingEntry> acentries;
                    TxItems txOrdered = OrderedTxItems(acentries);
                    for (TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                    {
                        CWalletTx *const pwtx = (*it).second.first;
                        if (pwtx == &wtx)
                            continue;

                        CAccountingEntry *const pacentry = (*it).second.second;
                        int64_t nSmartTime;
                        if (pwtx)
                        {
                            nSmartTime = pwtx->nTimeSmart;
                            if (!nSmartTime)
                                nSmartTime = pwtx->nTimeReceived;
                        } else
                        {
                            nSmartTime = pacentry->nTime;
                        };

                        if (nSmartTime <= latestTolerated)
                        {
                            latestEntry = nSmartTime;
                            if (nSmartTime > latestNow)
                                latestNow = nSmartTime;
                            break;
                        };
                    };

                    wtx.nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
                } else
                {
                    LogPrintf("AddToWallet() : found %s in block %s not in index\n",
                        hashIn.ToString().substr(0,10).c_str(),
                        wtxIn.hashBlock.ToString().c_str());
                };
            };
        };

        bool fUpdated = false;

        if (!fInsertedNew)
        {
            // Merge
            if (wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            };

            if (wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
            {
                wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            };

            if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            };

            fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
        };

        //// debug print
        LogPrintf("AddToWallet() %s  %s%s\n", hashIn.ToString().substr(0,10).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        // Write to disk
        if (fInsertedNew || fUpdated)
        {
            if (!wtx.WriteToDisk())
                return false;
        };

        /*

        if (!fHaveGUI)
        {
            // If default receiving address gets used, replace it with a new one
            if (vchDefaultKey.IsValid())
            {
                CScript scriptDefaultKey;
                scriptDefaultKey.SetDestination(vchDefaultKey.GetID());
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (txout.scriptPubKey == scriptDefaultKey)
                    {
                        CPubKey newDefaultKey;
                        if (GetKeyFromPool(newDefaultKey, false))
                        {
                            SetDefaultKey(newDefaultKey);
                            SetAddressBookName(vchDefaultKey.GetID(), "");
                        };
                    };
                };
            };
        };
        */

        // since AddToWallet is called directly for self-originating transactions, check for consumption of own coins
        WalletUpdateSpent(wtx, (wtxIn.hashBlock != 0));

        // Notify UI of new or updated transaction
        NotifyTransactionChanged(this, hashIn, fInsertedNew ? CT_NEW : CT_UPDATED);

        if (nNodeMode == NT_THIN
            && fInsertedNew == CT_NEW
            && pBloomFilter
            && (pBloomFilter->nFlags & BLOOM_UPDATE_MASK) == BLOOM_UPDATE_ALL)
        {
            uint32_t nAdded = 0;

            // -- add unspent outputs to bloom filters
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
            {
                if (wtx.nVersion == ANON_TXN_VERSION
                    && txin.IsAnonInput())
                    continue;

                WalletTxMap::iterator mi = mapWallet.find(txin.prevout.hash);
                if (mi == mapWallet.end())
                    continue;

                CWalletTx& wtxPrev = (*mi).second;

                if (txin.prevout.n >= wtxPrev.vout.size())
                {
                    LogPrintf("AddToWallet(): bad wtx %s\n", wtxPrev.GetHash().ToString().c_str());
                } else
                if (!wtxPrev.IsSpent(txin.prevout.n) && IsMine(wtxPrev.vout[txin.prevout.n]))
                {

                    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
                    stream << txin.prevout;
                    std::vector<unsigned char> vData(stream.begin(), stream.end());
                    AddDataToMerkleFilters(vData);
                    nAdded++;
                };
            };

            if (fDebug)
                LogPrintf("AddToWallet() Added %u outputs to bloom filters.\n", nAdded);
        }

        // notify an external script when a wallet transaction comes in or is updated
        std::string strCmd = GetArg("-walletnotify", "");

        if (!strCmd.empty())
        {
            boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        };

    }
    return true;
}

static int GetBlockHeightFromHash(const uint256& blockHash)
{
    if (blockHash == 0)
        return 0;

    if (nNodeMode == NT_FULL)
    {
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end())
            return 0;
        return mi->second->nHeight;
    } else
    {
        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(blockHash);
        if (mi == mapBlockThinIndex.end()
            && !fThinFullIndex
            && pindexRear)
        {
            CTxDB txdb("r");
            CDiskBlockThinIndex diskindex;
            if (txdb.ReadBlockThinIndex(blockHash, diskindex)
                || diskindex.hashNext != 0)
            {
                return diskindex.nHeight;
            };
        } else
        {
            return mi->second->nHeight;
        };
    };

    return 0;
}

// Add a transaction to the wallet, or update it.
// pblock is optional, but should be provided if the transaction is known to be in a block.
// If fUpdate is true, existing transactions will be updated.
bool CWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const uint256& hash, const void* pblock, bool fUpdate, bool fFindBlock)
{
    //LogPrintf("AddToWalletIfInvolvingMe() %s\n", hash.ToString().c_str()); // happens often

    //uint256 hash = tx.GetHash();
    {
        LOCK(cs_wallet);
        bool fExisted = mapWallet.count(hash);
        if (fExisted && !fUpdate)
        {
            return false;
        };

        mapValue_t mapNarr;

        bool fIsMine = false;
        if(!tx.IsCoinBase() && !tx.IsCoinStake())
        {
            // Skip transactions that we know wouldn't be stealth...
            FindStealthTransactions(tx, mapNarr);
        }

        if (tx.nVersion == ANON_TXN_VERSION)
        {
            LOCK(cs_main); // cs_wallet is already locked
            CWalletDB walletdb(strWalletFile, "cr+");
            CTxDB txdb("cr+");

            uint256 blockHash = (pblock ? (nNodeMode == NT_FULL ? ((CBlock*)pblock)->GetHash() : *(uint256*)pblock) : 0);

            walletdb.TxnBegin();
            txdb.TxnBegin();
            std::vector<WalletTxMap::iterator> vUpdatedTxns;
            std::map<int64_t, CAnonBlockStat> mapAnonBlockStat;
            if (!ProcessAnonTransaction(&walletdb, &txdb, tx, blockHash, fIsMine, mapNarr, vUpdatedTxns, mapAnonBlockStat))
            {
                LogPrintf("ProcessAnonTransaction failed %s\n", hash.ToString().c_str());
                walletdb.TxnAbort();
                txdb.TxnAbort();
                return false;
            } else
            {
                walletdb.TxnCommit();
                txdb.TxnCommit();
                int nBlockHeight = GetBlockHeightFromHash(blockHash);
                AddToAnonBlockStats(mapAnonBlockStat, nBlockHeight);
                for (std::vector<WalletTxMap::iterator>::iterator it = vUpdatedTxns.begin();
                    it != vUpdatedTxns.end(); ++it)
                    NotifyTransactionChanged(this, (*it)->first, CT_UPDATED);
            };
        };

        if (fExisted || fIsMine || IsMine(tx) || IsFromMe(tx))
        {
            CWalletTx wtx(this, tx);

            if (!mapNarr.empty())
                wtx.mapValue.insert(mapNarr.begin(), mapNarr.end());

            // Get merkle branch if transaction was found in a block
            if (nNodeMode == NT_FULL)
            {
                const CBlock* pcblock = (CBlock*)pblock;
                if (pcblock)
                    wtx.SetMerkleBranch(pcblock);
            } else
            {
                const uint256* pblockhash = (uint256*)pblock;

                if (pblockhash)
                    wtx.hashBlock = *pblockhash;
            };

            return AddToWallet(wtx, hash);
        } else
        {
            WalletUpdateSpent(tx);
        };
    }
    return false;
}

void CWallet::AddToAnonBlockStats(const std::map<int64_t, CAnonBlockStat>& mapAnonBlockStat, int nBlockHeight)
{
    if(!nBlockHeight)
        return;

    for (const auto & [nValue, newAnonBlockStat] : mapAnonBlockStat)
    {
        CAnonBlockStat& anonBlockStat = mapAnonBlockStats[nBlockHeight][nValue];
        anonBlockStat.nSpends += newAnonBlockStat.nSpends;
        anonBlockStat.nOutputs += newAnonBlockStat.nOutputs;
        anonBlockStat.nStakingOutputs += newAnonBlockStat.nStakingOutputs;
        anonBlockStat.nCompromisedOutputs += newAnonBlockStat.nCompromisedOutputs;
    }
}


bool CWallet::EraseFromWallet(uint256 hash)
{
    if (!fFileBacked)
        return false;

    {
        LOCK(cs_wallet);
        if (mapWallet.erase(hash))
            CWalletDB(strWalletFile).EraseTx(hash);
    }
    return true;
}


bool CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        WalletTxMap::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return true;
        };
    }
    return false;
}

int64_t CWallet::GetDebit(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        WalletTxMap::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return prev.vout[txin.prevout.n].nValue;
        };
    }
    return 0;
}

int64_t CWallet::GetSpectreDebit(const CTxIn& txin) const
{
    if (!txin.IsAnonInput())
        return 0;

    // - amount of owned spectre decreased
    // TODO: store links in memory

    {
        LOCK(cs_wallet);

        CWalletDB walletdb(strWalletFile, "r");

        std::vector<uint8_t> vchImage;
        txin.ExtractKeyImage(vchImage);

        COwnedAnonOutput oao;
        if (!walletdb.ReadOwnedAnonOutput(vchImage, oao))
            return 0;
        //return oao.nValue

        WalletTxMap::const_iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (oao.outpoint.n < prev.vout.size())
                return prev.vout[oao.outpoint.n].nValue;
        };

    }

    return 0;
};

int64_t CWallet::GetSpectreCredit(const CTxOut& txout) const
{
    if (!txout.IsAnonOutput())
        return 0;

    // TODO: store links in memory

    {
        LOCK(cs_wallet);

        CWalletDB walletdb(strWalletFile, "r");

        CPubKey pkCoin = txout.ExtractAnonPk();

        COutPoint outpoint;
        std::vector<uint8_t> vchImage;
        if (walletdb.ReadOwnedAnonOutputLink(pkCoin, vchImage))
        {
            COwnedAnonOutput oao;
            if (!walletdb.ReadOwnedAnonOutput(vchImage, oao))
                return 0;

            outpoint = oao.outpoint;
        }
        else
        {
            if (!IsCrypted())
                return 0;

            // - tokens received with locked wallet won't have oao until wallet unlocked
            CKeyID ckCoinId = pkCoin.GetID();
            CLockedAnonOutput lockedAo;
            if (!walletdb.ReadLockedAnonOutput(ckCoinId, lockedAo))
                return 0;

            outpoint = lockedAo.outpoint;
        };


        WalletTxMap::const_iterator mi = mapWallet.find(outpoint.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (outpoint.n < prev.vout.size())
            {
                return prev.vout[outpoint.n].nValue;
            };
        };
    } // cs_wallet

    return 0;
};

bool CWallet::IsChange(const CTxOut& txout) const
{
    CTxDestination address;

    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a TX_PUBKEYHASH that is mine but isn't in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (txout.IsAnonOutput()) {
        // TODO ExtractDestination does currently not support anonoutputs
        CKeyID ckidD = txout.ExtractAnonPk().GetID();

        bool fIsMine = HaveKey(ckidD);
        address = ckidD;
        LOCK(cs_wallet);
        if (fIsMine && !mapAddressBook.count(ckidD))
            return true;
    }
    else if (ExtractDestination(txout.scriptPubKey, address) && IsDestMine(*this, address))
    {
        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
    };

    return false;
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{
    // Returns -1 if it wasn't being tracked
    int nRequests = -1;

    {
        LOCK(pwallet->cs_wallet);
        if (IsCoinBase() || IsCoinStake())
        {
            // Generated block
            if (hashBlock != 0)
            {
                std::map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                if (mi != pwallet->mapRequestCount.end())
                    nRequests = (*mi).second;
            };
        } else
        {
            // Did anyone request this transaction?
            map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
            if (mi != pwallet->mapRequestCount.end())
            {
                nRequests = (*mi).second;

                // How about the block it's in?
                if (nRequests == 0 && hashBlock != 0)
                {
                    map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                    if (mi != pwallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                    else
                        nRequests = 1; // If it's in someone else's block it must have got out
                };
            };
        };
    }
    return nRequests;
}

void CWalletTx::GetDestinationDetails(list<CTxDestinationDetail>& listReceived, list<CTxDestinationDetail>& listSent, int64_t& nFee, string& strSentAccount) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    int64_t nDebit = GetDebit();
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        int64_t nValueOut = GetValueOut();
        nFee = nDebit - nValueOut;
    };

    Currency currencyDestination = PUBLIC;
    Currency currencySource = PUBLIC;

    if (IsAnonCoinStake())
        currencySource = PRIVATE;
    else
        for(const CTxIn& txin: vin)
        {
            if (txin.IsAnonInput())
            {
                currencySource = PRIVATE;
                break;
            }
        }

    bool generated = IsCoinBase() || IsCoinStake();

    // Sent/received.
    std::map<std::string, int64_t> mapStealthReceived;
    std::map<std::string, int64_t> mapStealthSent;
    std::map<std::string, std::vector<CTxDestination>> mapDestinationSubs;
    std::map<std::string, std::string> mapStealthNarration;

    // Only need to handle txouts if AT LEAST one of these is true:
    //   1) they debit from us (sent)
    //   2) the output is to us (received)
    for (uint32_t index = 0; index < vout.size(); ++index)
    {
        const CTxOut& txout = vout[index];

        // Skip special stake out
        if (txout.scriptPubKey.empty())
            continue;

        // Don't report 'change' txouts
        if (nDebit > 0 && !generated && pwallet->IsChange(txout))
            continue;

        if (nVersion == ANON_TXN_VERSION
            && txout.IsAnonOutput())
        {
            currencyDestination = PRIVATE;

            CKeyID ckidD = txout.ExtractAnonPk().GetID();

            bool fIsMine = pwallet->HaveKey(ckidD);
            if (nDebit <= 0 && !fIsMine)
                continue;

            std::string stealthAddress;
            {
                LOCK(pwallet->cs_wallet);
                if (pwallet->mapAddressBook.count(ckidD))
                    stealthAddress = pwallet->mapAddressBook.at(ckidD);
                else
                    stealthAddress = "UNKNOWN";
            }

            mapDestinationSubs[stealthAddress].push_back(ckidD);

            // If we are debited by the transaction, add the output as a "sent" entry
            if (nDebit > 0)
                mapStealthSent[stealthAddress] += txout.nValue;

            // If we are receiving the output, add it as a "received" entry
            if (fIsMine)
                mapStealthReceived[stealthAddress] += txout.nValue;

            // Get narration for stealth address
            std::string sNarr;
            if (GetNarration(index, sNarr))
                // TODO for UNKNOWN we should concat narrations if multiple are available
                mapStealthNarration[stealthAddress] = sNarr;

            continue;
        };

        opcodetype firstOpCode;
        CScript::const_iterator pc = txout.scriptPubKey.begin();
        if (txout.scriptPubKey.GetOp(pc, firstOpCode)
            && firstOpCode == OP_RETURN)
            continue;

        bool fIsMine = pwallet->IsMine(txout);
        if (nDebit <= 0 && !fIsMine)
            continue;

        // Get narration for output
        std::string sNarr;
        bool hasNarr = GetNarration(index, sNarr);

        // In either case, we need to get the destination address
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
        {
            LogPrintf("CWalletTx::GetDestinationDetails: Unknown transaction type found, txid %s\n",
                this->GetHash().ToString().c_str());
            address = CNoDestination();
        }
        else
        {
            LOCK(pwallet->cs_wallet);
            if (pwallet->mapAddressBook.count(address))
            {
                std::string stealthAddress = pwallet->mapAddressBook.at(address);
                if (IsStealthAddressMappingLabel(stealthAddress, false))
                {
                    mapDestinationSubs[stealthAddress].push_back(address);

                    // If we are debited by the transaction, add the output as a "sent" entry
                    if (nDebit > 0)
                        mapStealthSent[stealthAddress] += txout.nValue;

                    // If we are receiving the output, add it as a "received" entry
                    if (fIsMine)
                        mapStealthReceived[stealthAddress] += txout.nValue;

                    // Add narration to stealth output
                    if (hasNarr)
                        // TODO for UNKNOWN we should concat narrations if multiple are available
                        mapStealthNarration[stealthAddress] = sNarr;

                    continue;
                }
            }
        }

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.emplace_back(address, std::vector<CTxDestination>(), txout.nValue, std::optional<std::uint32_t>{index}, currencySource, sNarr);

        // If we are receiving the output, add it as a "received" entry
        if (fIsMine)
            listReceived.emplace_back(address, std::vector<CTxDestination>(), txout.nValue, std::optional<std::uint32_t>{index}, currencyDestination, sNarr);
    };

    for (const auto & [address, amount] : mapStealthSent) {
        CStealthAddress stealthAddress;
        if (pwallet->GetStealthAddress(address, stealthAddress))
            listSent.emplace_back(stealthAddress, mapDestinationSubs[address], amount, std::nullopt, currencySource, mapStealthNarration[address]);
        else
            listSent.emplace_back(CNoDestination(), mapDestinationSubs[address], amount, std::nullopt, currencySource, mapStealthNarration[address]);
    }

    for (const auto & [address, amount] : mapStealthReceived) {
        CStealthAddress stealthAddress;
        if (pwallet->GetStealthAddress(address, stealthAddress))
            listReceived.emplace_back(stealthAddress, mapDestinationSubs[address], amount, std::nullopt, currencyDestination, mapStealthNarration[address]);
        else
            listReceived.emplace_back(CNoDestination(), mapDestinationSubs[address], amount, std::nullopt, currencyDestination, mapStealthNarration[address]);
    }
}

bool CWallet::GetStealthAddress(const std::string& address, CStealthAddress& stealthAddressRet) const
{
    CStealthAddress sxAddr;
    std::string sAddressToCompare;

    if (IsAnonOrStealthMappingLabel(address))
        sAddressToCompare = address.substr(sAnonPrefix.length(), 16);
    else
    {
        if (sxAddr.SetEncoded(address))
            stealthAddressRet = sxAddr;
        else
            return false;
    }

    for (const CStealthAddress & sa : stealthAddresses)
    {
        if (!sAddressToCompare.empty())
        {
            std::string saEncoded = sa.Encoded();
            if (saEncoded.compare(0, sAddressToCompare.length(), sAddressToCompare) == 0)
            {
                stealthAddressRet = sa;
                return true;
            }
        }
        else if (sa == sxAddr)
        {
            stealthAddressRet = sa;
            return true;
        }
    }

    ExtKeyAccountMap::const_iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;
        if (ea->mapStealthKeys.size() < 1)
            continue;

        for (const auto& stealthKeys : ea->mapStealthKeys)
        {
            const CEKAStealthKey &aks = stealthKeys.second;

            if (!sAddressToCompare.empty())
            {
                 std::string saEncoded = aks.ToStealthAddress();
                 if (saEncoded.compare(0, sAddressToCompare.length(), sAddressToCompare) == 0)
                 {
                     sxAddr.SetEncoded(saEncoded);
                     sxAddr.label = aks.sLabel;
                     stealthAddressRet = sxAddr;
                     return true;
                 }
            }
            else
            {
                CStealthAddress cekaSxAddr;
                aks.SetSxAddr(cekaSxAddr);
                if (cekaSxAddr == sxAddr)
                {
                    sxAddr.label = aks.sLabel;
                    stealthAddressRet = sxAddr;
                    return true;
                }
            }
        }
    }

    return sxAddr.scan_pubkey.size() != 0;
}


void CWalletTx::GetAccountAmounts(const std::string& strAccount, int64_t& nReceived,
                                  int64_t& nSent, int64_t& nFee) const
{
    nReceived = nSent = nFee = 0;

    int64_t allFee;
    std::string strSentAccount;
    std::list<CTxDestinationDetail> listReceived;
    std::list<CTxDestinationDetail> listSent;
    GetDestinationDetails(listReceived, listSent, allFee, strSentAccount);

    if (strAccount == strSentAccount)
    {
        for(const auto & destination : listSent)
            nSent += destination.amount;
        nFee = allFee;
    };

    {
        LOCK(pwallet->cs_wallet);
        for(const auto & destination : listReceived)
        {
            if (pwallet->mapAddressBook.count(destination.address))
            {
                std::map<CTxDestination, std::string>::const_iterator mi = pwallet->mapAddressBook.find(destination.address);
                if (mi != pwallet->mapAddressBook.end() && (*mi).second == strAccount)
                    nReceived += destination.amount;
            } else
            if (strAccount.empty())
            {
                nReceived += destination.amount;
            };
        };
    } // pwallet->cs_wallet
}

void CWalletTx::AddSupportingTransactions(CTxDB& txdb)
{
    vtxPrev.clear();

    const int COPY_DEPTH = 3;
    if (SetMerkleBranch() < COPY_DEPTH)
    {
        std::vector<uint256> vWorkQueue;
        BOOST_FOREACH(const CTxIn& txin, vin)
            vWorkQueue.push_back(txin.prevout.hash);

        // This critsect is OK because txdb is already open
        {
            LOCK(pwallet->cs_wallet);
            map<uint256, const CMerkleTx*> mapWalletPrev;
            set<uint256> setAlreadyDone;
            for (unsigned int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hash = vWorkQueue[i];
                if (setAlreadyDone.count(hash))
                    continue;
                setAlreadyDone.insert(hash);

                CMerkleTx tx;
                WalletTxMap::const_iterator mi = pwallet->mapWallet.find(hash);
                if (mi != pwallet->mapWallet.end())
                {
                    tx = (*mi).second;
                    BOOST_FOREACH(const CMerkleTx& txWalletPrev, (*mi).second.vtxPrev)
                        mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;
                } else
                if (mapWalletPrev.count(hash))
                {
                    tx = *mapWalletPrev[hash];
                } else
                if (txdb.ReadDiskTx(hash, tx))
                {
                    ;
                } else
                {
                    LogPrintf("ERROR: AddSupportingTransactions() : unsupported transaction\n");
                    continue;
                };

                int nDepth = tx.SetMerkleBranch();
                vtxPrev.push_back(tx);

                if (nDepth < COPY_DEPTH)
                {
                    BOOST_FOREACH(const CTxIn& txin, tx.vin)
                        vWorkQueue.push_back(txin.prevout.hash);
                };
            };
        } // pwallet->cs_wallet
    };

    reverse(vtxPrev.begin(), vtxPrev.end());
}

bool CWalletTx::WriteToDisk()
{
    return CWalletDB(pwallet->strWalletFile).WriteTx(GetHash(), *this);
}

uint32_t CWallet::ClearWalletTransactions(bool onlyUnaccepted)
{
    uint32_t nTransactions = 0;
    char cbuf[256];
    {
        LOCK2(cs_main, cs_wallet);

        CWalletDB walletdb(strWalletFile);
        walletdb.TxnBegin();
        Dbc* pcursor = walletdb.GetTxnCursor();
        if (!pcursor)
            throw std::runtime_error("Cannot get wallet DB cursor");

        Dbt datKey;
        Dbt datValue;

        datKey.set_flags(DB_DBT_USERMEM);
        datValue.set_flags(DB_DBT_USERMEM);

        std::vector<unsigned char> vchKey;
        std::vector<unsigned char> vchType;
        std::vector<unsigned char> vchKeyData;
        std::vector<unsigned char> vchValueData;

        vchKeyData.resize(100);
        vchValueData.resize(100);

        datKey.set_ulen(vchKeyData.size());
        datKey.set_data(&vchKeyData[0]);

        datValue.set_ulen(vchValueData.size());
        datValue.set_data(&vchValueData[0]);

        unsigned int fFlags = DB_NEXT; // same as using DB_FIRST for new cursor
        while (true)
        {
            int ret = pcursor->get(&datKey, &datValue, fFlags);

            if (ret == ENOMEM
                || ret == DB_BUFFER_SMALL)
            {
                if (datKey.get_size() > datKey.get_ulen())
                {
                    vchKeyData.resize(datKey.get_size());
                    datKey.set_ulen(vchKeyData.size());
                    datKey.set_data(&vchKeyData[0]);
                };

                if (datValue.get_size() > datValue.get_ulen())
                {
                    vchValueData.resize(datValue.get_size());
                    datValue.set_ulen(vchValueData.size());
                    datValue.set_data(&vchValueData[0]);
                };
                // -- try once more, when DB_BUFFER_SMALL cursor is not expected to move
                ret = pcursor->get(&datKey, &datValue, fFlags);
            };

            if (ret == DB_NOTFOUND)
                break;
            else
            if (datKey.get_data() == NULL || datValue.get_data() == NULL
                || ret != 0)
            {
                snprintf(cbuf, sizeof(cbuf), "wallet DB error %d, %s", ret, db_strerror(ret));
                throw std::runtime_error(cbuf);
            };

            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            ssValue.SetType(SER_DISK);
            ssValue.clear();
            ssValue.write((char*)datKey.get_data(), datKey.get_size());

            ssValue >> vchType;


            std::string strType(vchType.begin(), vchType.end());

            //LogPrintf("strType %s\n", strType.c_str());

            if (strType == "tx")
            {
                uint256 hash;
                ssValue >> hash;

                if (onlyUnaccepted)
                {
                    const CWalletTx& wtx = mapWallet[hash];
                    if (!wtx.IsInMainChain())
                    {
                        if ((ret = pcursor->del(0)) != 0)
                        {
                            LogPrintf("Delete transaction failed %d, %s\n", ret, db_strerror(ret));
                            continue;
                        }
                        mapWallet.erase(hash);
                        NotifyTransactionChanged(this, hash, CT_DELETED);
                        nTransactions++;
                    }
                    continue;
                }

                if ((ret = pcursor->del(0)) != 0)
                {
                    LogPrintf("Delete transaction failed %d, %s\n", ret, db_strerror(ret));
                    continue;
                }

                mapWallet.erase(hash);
                NotifyTransactionChanged(this, hash, CT_DELETED);

                nTransactions++;
            };
        };
        pcursor->close();
        walletdb.TxnCommit();

        //pwalletMain->mapWallet.clear();

        if (nNodeMode == NT_THIN)
        {
            // reset LastFilteredHeight
            walletdb.WriteLastFilteredHeight(0);
        }
    }

    return nTransactions;
}

// Scan the block chain (starting in pindexStart) for transactions
// from or to us. If fUpdate is true, found transactions that already
// exist in the wallet will be updated.
int CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate, std::function<bool (const int&, const int&, const int&)> funcProgress, int progressBatchSize)
{
    if (fDebug)
        LogPrintf("ScanForWalletTransactions()\n");

    if (nNodeMode != NT_FULL)
    {
        LogPrintf("Error: CWallet::ScanForWalletTransactions() must be run in full mode.\n");
        return 0;
    };

    int ret = 0;
    int nCurBestHeight = nBestHeight;

    fReindexing = true;
    // Note: Every block is scanned to rebuild the anonymous transaction cache
    // therefore nTimeFirstKey (time of first wallet key) is not considered as filter
    CBlockIndex* pindex = pindexStart;
    {
        LOCK2(cs_main, cs_wallet);

        // call progress callback on start
        if (funcProgress) funcProgress(pindex->nHeight, nCurBestHeight, ret);

        CTxDB txdb;
        while (pindex)
        {
            CBlock block;
            block.ReadFromDisk(pindex, true);
            nBestHeight = pindex->nHeight;

            BOOST_FOREACH(CTransaction& tx, block.vtx)
            {
                uint256 hash = tx.GetHash();
                if (AddToWalletIfInvolvingMe(tx, hash, &block, fUpdate))
                    ret++;
            };
            if (funcProgress && pindex->nHeight % progressBatchSize == 0 && !funcProgress(pindex->nHeight, nCurBestHeight, ret)) {
                // abort scanning indicated
                break;
            };
            pindex = pindex->pnext;
        };

        // call progress callback on end
        if (funcProgress) funcProgress(nCurBestHeight, nCurBestHeight, ret);

    } // cs_main, cs_wallet

    nBestHeight = nCurBestHeight;
    fReindexing = false;

    // Make sure anon cache reflects restored nBestHeight
    if (!CacheAnonStats(nBestHeight))
        LogPrintf("ScanForWalletTransactions() : CacheAnonStats() failed.\n");

    return ret;
}

void CWallet::ReacceptWalletTransactions()
{
    if (fDebug)
        LogPrintf("ReacceptWalletTransactions()\n");

    CTxDB txdb("r");

    if (nNodeMode == NT_THIN)
    {
        for (WalletTxMap::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            CWalletTx& wtx = (*it).second;

            if ((wtx.IsCoinBase() && wtx.IsSpent(0))
                || (wtx.IsCoinStake() && wtx.IsSpent(1)))
                continue;

            std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtx.hashBlock);
            if (mi == mapBlockThinIndex.end())
            {
                if (!fThinFullIndex)
                {
                    CDiskBlockThinIndex diskindex;
                    if (txdb.ReadBlockThinIndex(wtx.hashBlock, diskindex)
                        && diskindex.hashNext != 0)
                        continue; // block is in db and in main chain
                };

                // Re-accept any txes of ours that aren't already in a block
                if (!(wtx.IsCoinBase() || wtx.IsCoinStake()))
                    wtx.AcceptWalletTransaction(txdb);

                continue;
            };
        };
        return;
    };


    bool fRepeat = true;
    while (fRepeat)
    {
        LOCK2(cs_main, cs_wallet);

        fRepeat = false;
        std::vector<CDiskTxPos> vMissingTx;
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            if ((wtx.IsCoinBase() && wtx.IsSpent(0))
                || (wtx.IsCoinStake() && wtx.IsSpent(1)))
                continue;

            CTxIndex txindex;
            bool fUpdated = false;
            if (txdb.ReadTxIndex(wtx.GetHash(), txindex))
            {
                // Update fSpent if a tx got spent somewhere else by a copy of wallet.dat
                if (txindex.vSpent.size() != wtx.vout.size())
                {
                    LogPrintf("ERROR: ReacceptWalletTransactions() : txindex.vSpent.size() %u != wtx.vout.size() %u\n", txindex.vSpent.size(), wtx.vout.size());
                    continue;
                };

                for (unsigned int i = 0; i < txindex.vSpent.size(); i++)
                {
                    if (wtx.IsSpent(i))
                        continue;

                    if (!txindex.vSpent[i].IsNull() && IsMine(wtx.vout[i]))
                    {
                        wtx.MarkSpent(i);
                        fUpdated = true;
                        vMissingTx.push_back(txindex.vSpent[i]);
                    };
                };

                if (fUpdated)
                {
                    LogPrintf("ReacceptWalletTransactions found spent coin %s ALIAS (public) %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkDirty();
                    wtx.WriteToDisk();
                };
            } else
            {
                // Re-accept any txes of ours that aren't already in a block
                if (!(wtx.IsCoinBase() || wtx.IsCoinStake()))
                    wtx.AcceptWalletTransaction(txdb);
            };
        };

        if (!vMissingTx.empty())
        {
            // TODO: optimize this to scan just part of the block chain?
            if (ScanForWalletTransactions(pindexGenesisBlock))
                fRepeat = true;  // Found missing transactions: re-do re-accept.
        };
    };
}

void CWalletTx::RelayWalletTransaction(CTxDB& txdb)
{
    BOOST_FOREACH(const CMerkleTx& tx, vtxPrev)
    {
        if (!(tx.IsCoinBase() || tx.IsCoinStake()))
        {
            uint256 hash = tx.GetHash();
            if (!txdb.ContainsTx(hash))
                RelayTransaction((CTransaction)tx, hash);
        };
    };

    if (!(IsCoinBase() || IsCoinStake()))
    {
        uint256 hash = GetHash();
        if (!txdb.ContainsTx(hash))
        {
            LogPrintf("Relaying wtx %s\n", hash.ToString().substr(0,10).c_str());
            RelayTransaction((CTransaction)*this, hash);
        };
    };
}

void CWalletTx::RelayWalletTransaction()
{
   CTxDB txdb("r");
   RelayWalletTransaction(txdb);
}

void CWallet::ResendWalletTransactions(bool fForce)
{
    if (!fForce)
    {
        // Do this infrequently and randomly to avoid giving away
        // that these are our transactions.
        static int64_t nNextTime = 0;
        if (GetTime() < nNextTime)
            return;
        bool fFirst = (nNextTime == 0);
        nNextTime = GetTime() + GetRand(30 * 60);
        if (fFirst)
            return;

        // Only do it if there's been a new block since last time
        static int64_t nLastTime;
        if (nTimeBestReceived < nLastTime)
            return;
        nLastTime = GetTime();
    };

    // Rebroadcast any of our txes that aren't in a block yet
    LogPrintf("ResendWalletTransactions()\n");
    CTxDB txdb("r");

    multimap<unsigned int, CWalletTx*> mapSorted;
    {
        LOCK(cs_wallet);
        // Sort them in chronological order
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            // Don't rebroadcast until it's had plenty of time that
            // it should have gotten in already by now.
            if (fForce || nTimeBestReceived - (int64_t)wtx.nTimeReceived > 5 * 60)
                mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
        };
    } // cs_wallet

    BOOST_FOREACH(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
    {
        CWalletTx& wtx = *item.second;
        if (wtx.CheckTransaction())
            wtx.RelayWalletTransaction(txdb);
        else
            LogPrintf("ResendWalletTransactions() : CheckTransaction failed for transaction %s\n", wtx.GetHash().ToString().c_str());
    };

}






//////////////////////////////////////////////////////////////////////////////
//
// Actions
//


int64_t CWallet::GetBalance() const
{
    int64_t nTotal = 0;

    {
        LOCK2(cs_main, cs_wallet);
        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableCredit();
        };
    }

    return nTotal;
}

int64_t CWallet::GetSpectreBalance() const
{
    int64_t nTotal = 0;

    {
        LOCK2(cs_main, cs_wallet);
        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->nVersion == ANON_TXN_VERSION && pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableSpectreCredit();
        };
    }

    return nTotal;
};


int64_t CWallet::GetUnconfirmedBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0))
                nTotal += pcoin->GetAvailableCredit();
        };
    }
    return nTotal;
}

int64_t CWallet::GetUnconfirmedSpectreBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->nVersion != ANON_TXN_VERSION || pcoin->IsCoinBase() || pcoin->IsCoinStake())
                continue;
            if (!pcoin->IsFinal() || (pcoin->GetDepthInMainChain() >= 0 && pcoin->GetDepthInMainChain() < MIN_ANON_SPEND_DEPTH))
            {
                int64_t nSPEC = 0, nSpectre = 0;
                if (CWallet::GetCredit(*pcoin, nSPEC, nSpectre))
                    nTotal += nSpectre;
            }
        };
    }
    return nTotal;
}


int64_t CWallet::GetImmatureBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx& pcoin = (*it).second;
            if (pcoin.IsCoinBase() && pcoin.GetBlocksToMaturity() > 0 && pcoin.IsInMainChain())
                nTotal += pcoin.GetCredit();
        }
    }
    return nTotal;
}

int64_t CWallet::GetImmatureSpectreBalance() const
{
    return 0; // not used
}

// populate vCoins with vector of spendable COutputs
void CWallet::AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);
        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if (!pcoin->IsFinal())
                continue;

            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinBase()||pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
                if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue >= nMinimumInputValue &&
                (!coinControl || !coinControl->HasSelected() || coinControl->IsSelected((*it).first, i)))
                    vCoins.push_back(COutput(pcoin, i, nDepth));

        }
    }
}

void CWallet::AvailableCoinsForStaking(std::vector<COutput>& vCoins, unsigned int nSpendTime) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);

        bool fPoSv3 = Params().IsProtocolV3(nBestHeight);

        for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            // Filtering by tx timestamp instead of block timestamp may give false positives but never false negatives
            if ((!fPoSv3 || !Params().IsForkV3(nSpendTime)) && pcoin->nTime + nStakeMinAge > nSpendTime)
                continue;

            if (pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 1 || (fPoSv3 && nDepth < Params().GetStakeMinConfirmations(nSpendTime)))
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->nVersion == ANON_TXN_VERSION
                    && pcoin->vout[i].IsAnonOutput())
                    continue;
                if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue >= nMinimumInputValue)
                    vCoins.push_back(COutput(pcoin, i, nDepth));
            };
        };
    }
}

static void ApproximateBestSubset(
    std::vector<pair<int64_t, pair<const CWalletTx*,unsigned int> > >vValue, int64_t nTotalLower, int64_t nTargetValue,
    std::vector<char>& vfBest, int64_t& nBest, int iterations = 1000)
{
    std::vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64_t nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                if (nPass == 0 ? rand() % 2 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

// ppcoin: total coins staked (non-spendable until maturity)
int64_t CWallet::GetStake() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
        {
            int64_t nSPEC = 0, nSpectre = 0;
            if (CWallet::GetCredit(*pcoin, nSPEC, nSpectre))
                nTotal += nSPEC;
        }
    };
    return nTotal;
}

int64_t CWallet::GetSpectreStake() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->nVersion != ANON_TXN_VERSION)
            continue;
        if (pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
        {
            int64_t nSPEC = 0, nSpectre = 0;
            if (CWallet::GetCredit(*pcoin, nSPEC, nSpectre))
                nTotal += nSpectre;
        }
    };
    return nTotal;
}

int64_t CWallet::GetNewMint() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin);
    };
    return nTotal;
}

bool CWallet::SelectCoinsMinConf(int64_t nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    pair<int64_t, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<int64_t>::max();
    coinLowestLarger.second.first = NULL;
    std::vector<std::pair<int64_t, std::pair<const CWalletTx*,unsigned int> > > vValue;
    int64_t nTotalLower = 0;

// Removed with c++17, see https://en.cppreference.com/w/cpp/algorithm/random_shuffle
//    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);
    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(vCoins.begin(), vCoins.end(), urng);

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;

        if (output.nDepth < (pcoin->IsFromMe() ? nConfMine : nConfTheirs))
            continue;

        int i = output.i;

        // Follow the timestamp rules
        if (pcoin->nTime > nSpendTime)
            continue;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            return true;
        }
        else if (n < nTargetValue + CENT)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend(), CompareValueOnly());
    std::vector<char> vfBest;
    int64_t nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest, 1000);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + CENT)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + CENT, vfBest, nBest, 1000);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second.first &&
        ((nBest != nTargetValue && nBest < nTargetValue + CENT) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    }
    else
    {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        if (fDebug && GetBoolArg("-printpriority"))
        {
            //// debug print
            LogPrintf("SelectCoins() best subset: ");
            for (unsigned int i = 0; i < vValue.size(); i++)
                if (vfBest[i])
                    LogPrintf("%s ", FormatMoney(vValue[i].first).c_str());
            LogPrintf("total %s\n", FormatMoney(nBest).c_str());
        }
    }

    return true;
}

bool CWallet::SelectCoins(int64_t nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, const CCoinControl* coinControl) const
{
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coinControl && coinControl->HasSelected())
    {
        BOOST_FOREACH(const COutput& out, vCoins)
        {
            nValueRet += out.tx->vout[out.i].nValue;
            setCoinsRet.insert(make_pair(out.tx, out.i));
        }
        return (nValueRet >= nTargetValue);
    }

    boost::function<bool (const CWallet*, int64_t, unsigned int, int, int, std::vector<COutput>, std::set<std::pair<const CWalletTx*,unsigned int> >&, int64_t&)> f = &CWallet::SelectCoinsMinConf;

    return (f(this, nTargetValue, nSpendTime, 1, 10, vCoins, setCoinsRet, nValueRet) ||
            f(this, nTargetValue, nSpendTime, 1, 1, vCoins, setCoinsRet, nValueRet) ||
            f(this, nTargetValue, nSpendTime, 0, 1, vCoins, setCoinsRet, nValueRet));
}

// Select some coins without random shuffle or best subset approximation
bool CWallet::SelectCoinsForStaking(int64_t nMaxAmount, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    std::vector<COutput> vCoins;
    AvailableCoinsForStaking(vCoins, nSpendTime);

    setCoinsRet.clear();
    nValueRet = 0;

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;
        int i = output.i;
        int64_t n = pcoin->vout[i].nValue;

        // Skip if the selection of the coin would overflow the target
        if (nValueRet + n > nMaxAmount)
            continue;

        pair<int64_t, pair<const CWalletTx*, unsigned int> > coin = make_pair(n, make_pair(pcoin, i));
        setCoinsRet.insert(coin.second);
        nValueRet += coin.first;
    }

    return true;
}


bool CWallet::CreateTransaction(const std::vector<std::pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, int64_t& nFeeRet, int32_t& nChangePos, const CCoinControl* coinControl)
{
    int64_t nValue = 0;
    BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
    {
        if (nValue < 0)
            return false;
        nValue += s.second;
    };

    if (vecSend.empty() || nValue < 0)
        return false;

    wtxNew.BindWallet(this);

    {
        LOCK2(cs_main, cs_wallet);
        // txdb must be opened before the mapWallet lock
        CTxDB txdb("r");
        {
            nFeeRet = nTransactionFee;
            while (true)
            {
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64_t nTotalValue = nValue + nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                BOOST_FOREACH(const PAIRTYPE(CScript, int64_t)& s, vecSend)
                {
                    wtxNew.vout.push_back(CTxOut(s.second, s.first));
                };

                // Choose coins to use
                set<pair<const CWalletTx*,unsigned int> > setCoins;
                int64_t nValueIn = 0;
                if (!SelectCoins(nTotalValue, wtxNew.nTime, setCoins, nValueIn, coinControl))
                    return false;

                BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
                {
                    int64_t nCredit = pcoin.first->vout[pcoin.second].nValue;
                    dPriority += (double)nCredit * pcoin.first->GetDepthInMainChain();
                };

                int64_t nChange = nValueIn - nValue - nFeeRet;
                // if sub-cent change is required, the fee must be raised to at least MIN_TX_FEE
                // or until nChange becomes zero
                // NOTE: this depends on the exact behaviour of GetMinFee
                if (nFeeRet < nMinTxFee && nChange > 0 && nChange < CENT)
                {
                    int64_t nMoveToFee = min(nChange, nMinTxFee - nFeeRet);
                    nChange -= nMoveToFee;
                    nFeeRet += nMoveToFee;
                };

                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-bitcoin-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                    {
                        scriptChange.SetDestination(coinControl->destChange);
                    } else
                    {
                        // no coin control: send change to newly generated address

                        // Note: We use a new key here to keep it from being obvious which side is the change.


                        // Use the next key in the internal chain of the default account.
                        // TODO: send in more parameters so GetChangeAddress can pick the account to derive from.

                        CPubKey vchPubKey;
                        if (0 != GetChangeAddress(vchPubKey))
                            return false;

                        scriptChange.SetDestination(vchPubKey.GetID());
                    };

                    // Insert change txn at random position:
                    std::vector<CTxOut>::iterator position = wtxNew.vout.begin()+GetRandInt(wtxNew.vout.size() + 1);

                    // -- don't put change output between value and narration outputs
                    if (position > wtxNew.vout.begin() && position < wtxNew.vout.end())
                    {
                        while (position > wtxNew.vout.begin())
                        {
                            if (position->nValue != 0)
                                break;
                            position--;
                        };
                    };
                    position = wtxNew.vout.insert(position, CTxOut(nChange, scriptChange));
                    nChangePos = std::distance(wtxNew.vout.begin(), position);
                };

                // Fill vin
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    wtxNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second));

                // Sign
                int nIn = 0;
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    if (!SignSignature(*this, *coin.first, wtxNew, nIn++))
                    {
                        LogPrintf("%s: Error SignSignature failed.\n", __func__);
                        return false;
                    };
                // Limit size
                unsigned int nBytes = ::GetSerializeSize(*(CTransaction*)&wtxNew, SER_NETWORK, PROTOCOL_VERSION);
                if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
                {
                    LogPrintf("%s: Error MAX_BLOCK_SIZE_GEN/5 limit hit.\n", __func__);
                    return false;
                };
                dPriority /= nBytes;

                // Check that enough fee is included
                int64_t nPayFee = nTransactionFee * (1 + (int64_t)nBytes / 1000);
                int64_t nMinFee = wtxNew.GetMinFee(1, GMF_SEND, nBytes);

                if (nFeeRet < max(nPayFee, nMinFee))
                {
                    nFeeRet = max(nPayFee, nMinFee);
                    continue;
                };

                // Fill vtxPrev by copying from previous transactions vtxPrev
                wtxNew.AddSupportingTransactions(txdb);
                wtxNew.fTimeReceivedIsTxTime = true;

                break;
            };
        }
    }
    return true;
}




bool CWallet::CreateTransaction(CScript scriptPubKey, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, int64_t& nFeeRet, const CCoinControl* coinControl)
{
    std::vector<std::pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    if (sNarr.length() > 0)
    {
        std::vector<uint8_t> vNarr(sNarr.c_str(), sNarr.c_str() + sNarr.length());
        std::vector<uint8_t> vNDesc;

        vNDesc.resize(2);
        vNDesc[0] = 'n';
        vNDesc[1] = 'p';

        CScript scriptN = CScript() << OP_RETURN << vNDesc << OP_RETURN << vNarr;

        vecSend.push_back(make_pair(scriptN, 0));
    };

    // -- CreateTransaction won't place change between value and narr output.
    //    narration output will be for preceding output

    int nChangePos;
    bool rv = CreateTransaction(vecSend, wtxNew, nFeeRet, nChangePos, coinControl);

    // -- narration will be added to mapValue later in FindStealthTransactions From CommitTransaction
    return rv;
}


bool CWallet::AddStealthAddress(CStealthAddress& sxAddr)
{
    LOCK(cs_wallet);

    // - must add before changing spend_secret
    stealthAddresses.insert(sxAddr);

    bool fOwned = sxAddr.scan_secret.size() == EC_SECRET_SIZE;

    if (fOwned)
    {
        // -- owned addresses can only be added when wallet is unlocked
        if (IsLocked())
        {
            LogPrintf("Error: CWallet::AddStealthAddress wallet must be unlocked.\n");
            stealthAddresses.erase(sxAddr);
            return false;
        };

        if (IsCrypted())
        {
            std::vector<unsigned char> vchCryptedSecret;
            CSecret vchSecret;
            vchSecret.resize(EC_SECRET_SIZE);
            memcpy(&vchSecret[0], &sxAddr.spend_secret[0], EC_SECRET_SIZE);

            uint256 iv = Hash(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end());
            if (!EncryptSecret(vMasterKey, vchSecret, iv, vchCryptedSecret))
            {
                LogPrintf("Error: Failed encrypting stealth key %s\n", sxAddr.Encoded().c_str());
                stealthAddresses.erase(sxAddr);
                return false;
            };
            sxAddr.spend_secret = vchCryptedSecret;
        };
    };


    bool rv = CWalletDB(strWalletFile).WriteStealthAddress(sxAddr);


    if (rv)
        NotifyAddressBookChanged(this, sxAddr, sxAddr.label, fOwned, CT_NEW, true);

    return rv;
}

bool CWallet::UnlockStealthAddresses(const CKeyingMaterial& vMasterKeyIn)
{
    // -- decrypt spend_secret of stealth addresses
    std::set<CStealthAddress>::iterator it;
    for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() < EC_SECRET_SIZE)
            continue; // stealth address is not owned

        // -- CStealthAddress are only sorted on spend_pubkey
        CStealthAddress &sxAddr = const_cast<CStealthAddress&>(*it);

        if (fDebug)
            LogPrintf("Decrypting stealth key %s\n", sxAddr.Encoded().c_str());

        CSecret vchSecret;
        uint256 iv = Hash(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end());
        if (!DecryptSecret(vMasterKeyIn, sxAddr.spend_secret, iv, vchSecret)
            || vchSecret.size() != EC_SECRET_SIZE)
        {
            LogPrintf("Error: Failed decrypting stealth key %s\n", sxAddr.Encoded().c_str());
            continue;
        };

        ec_secret testSecret;
        memcpy(&testSecret.e[0], &vchSecret[0], EC_SECRET_SIZE);
        ec_point pkSpendTest;

        if (SecretToPublicKey(testSecret, pkSpendTest) != 0
            || pkSpendTest != sxAddr.spend_pubkey)
        {
            LogPrintf("Error: Failed decrypting stealth key, public key mismatch %s\n", sxAddr.Encoded().c_str());
            continue;
        };

        sxAddr.spend_secret.resize(EC_SECRET_SIZE);
        memcpy(&sxAddr.spend_secret[0], &vchSecret[0], EC_SECRET_SIZE);
    };

    CryptedKeyMap::iterator mi = mapCryptedKeys.begin();
    for (; mi != mapCryptedKeys.end(); ++mi)
    {
        CPubKey &pubKey = (*mi).second.first;
        std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        if (vchCryptedSecret.size() != 0)
            continue;

        CKeyID ckid = pubKey.GetID();
        CBitcoinAddress addr(ckid);

        StealthKeyMetaMap::iterator mi = mapStealthKeyMeta.find(ckid);
        if (mi == mapStealthKeyMeta.end())
        {
            // -- could be an anon output
            if (fDebug)
                LogPrintf("Warning: No metadata found to add secret for %s\n", addr.ToString().c_str());
            continue;
        };

        CStealthKeyMetadata& sxKeyMeta = mi->second;

        CStealthAddress sxFind;
        sxFind.SetScanPubKey(sxKeyMeta.pkScan);

        std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxFind);
        if (si == stealthAddresses.end())
        {
            LogPrintf("No stealth key found to add secret for %s\n", addr.ToString().c_str());
            continue;
        };

        if (fDebug)
            LogPrintf("Expanding secret for %s\n", addr.ToString().c_str());

        ec_secret sSpendR;
        ec_secret sSpend;
        ec_secret sScan;

        if (si->spend_secret.size() != EC_SECRET_SIZE
            || si->scan_secret.size() != EC_SECRET_SIZE)
        {
            LogPrintf("Stealth address has no secret key for %s\n", addr.ToString().c_str());
            continue;
        };
        memcpy(&sScan.e[0], &si->scan_secret[0], EC_SECRET_SIZE);
        memcpy(&sSpend.e[0], &si->spend_secret[0], EC_SECRET_SIZE);

        ec_point pkEphem;;
        pkEphem.resize(sxKeyMeta.pkEphem.size());
        memcpy(&pkEphem[0], sxKeyMeta.pkEphem.begin(), sxKeyMeta.pkEphem.size());

        if (StealthSecretSpend(sScan, pkEphem, sSpend, sSpendR) != 0)
        {
            LogPrintf("StealthSecretSpend() failed.\n");
            continue;
        };

        CKey ckey;
        ckey.Set(&sSpendR.e[0], true);

        if (!ckey.IsValid())
        {
            LogPrintf("Reconstructed key is invalid.\n");
            continue;
        };

        CPubKey cpkT = ckey.GetPubKey(true);

        if (!cpkT.IsValid())
        {
            LogPrintf("%s: cpkT is invalid.\n", __func__);
            continue;
        };

        if (cpkT != pubKey)
        {
            LogPrintf("%s: Error: Generated secret does not match.\n", __func__);
            if (fDebug)
            {
                LogPrintf("cpkT   %s\n", HexStr(cpkT).c_str());
                LogPrintf("pubKey %s\n", HexStr(pubKey).c_str());
            };
            continue;
        };

        if (fDebug)
        {
            CKeyID keyID = cpkT.GetID();
            CBitcoinAddress coinAddress(keyID);
            LogPrintf("%s: Adding secret to key %s.\n", __func__, coinAddress.ToString().c_str());
        };

        if (!AddKeyPubKey(ckey, cpkT))
        {
            LogPrintf("%s: AddKeyPubKey failed.\n", __func__);
            continue;
        };

        if (!CWalletDB(strWalletFile).EraseStealthKeyMeta(ckid))
            LogPrintf("EraseStealthKeyMeta failed for %s\n", addr.ToString().c_str());
    };
    return true;
}

bool CWallet::UpdateStealthAddress(std::string &addr, std::string &label, bool addIfNotExist)
{
    if (fDebug)
        LogPrintf("%s: %s\n", __func__, addr.c_str());


    CStealthAddress sxAddr;

    if (!sxAddr.SetEncoded(addr))
        return error("%s: Invalid address.", __func__);

    LOCK(cs_wallet);

    CKeyID sxId = CPubKey(sxAddr.scan_pubkey).GetID();

    ExtKeyAccountMap::const_iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;

        if (ea->mapStealthKeys.size() < 1)
            continue;

        AccStealthKeyMap::iterator it = ea->mapStealthKeys.find(sxId);
        if (it != ea->mapStealthKeys.end())
        {
            CWalletDB wdb(strWalletFile);
            return (0 == ExtKeyUpdateStealthAddress(&wdb, ea, sxId, label));
        };
    };


    std::set<CStealthAddress>::iterator it;
    it = stealthAddresses.find(sxAddr);

    ChangeType nMode = CT_UPDATED;
    CStealthAddress sxFound;
    if (it == stealthAddresses.end())
    {
        if (addIfNotExist)
        {
            sxFound = sxAddr;
            sxFound.label = label;
            stealthAddresses.insert(sxFound);
            nMode = CT_NEW;
        } else
        {
            return error("%s: %s, not in set.", __func__, addr.c_str());;
        };
    } else
    {
        sxFound = const_cast<CStealthAddress&>(*it);

        if (sxFound.label == label)
        {
            // no change
            return true;
        };

        it->label = label; // update in .stealthAddresses

        if (sxFound.scan_secret.size() == EC_SECRET_SIZE)
        {
            // -- read from db to keep encryption
            CStealthAddress sxOwned;

            if (!CWalletDB(strWalletFile).ReadStealthAddress(sxFound))
            {
                error("%s: error - sxFound not in db.", __func__);
                return false;
            };
        };
    };

    sxFound.label = label;

    if (!CWalletDB(strWalletFile).WriteStealthAddress(sxFound))
    {
        return error("%s: %s WriteStealthAddress failed.", __func__, addr.c_str());
    };

    bool fOwned = sxFound.scan_secret.size() == EC_SECRET_SIZE;
    NotifyAddressBookChanged(this, sxFound, sxFound.label, fOwned, nMode, true);

    return true;
};

bool CWallet::FindStealthTransactions(const CTransaction& tx, mapValue_t& mapNarr)
{
    if (fDebug)
        LogPrintf("%s: tx: %s.\n", __func__, tx.GetHash().GetHex().c_str());

    mapNarr.clear();

    LOCK(cs_wallet);
    ec_secret sSpendR;
    ec_secret sSpend;
    ec_secret sScan;
    ec_secret sShared;

    ec_point pkExtracted;

    std::vector<uint8_t> vchEphemPK;
    std::vector<uint8_t> vchDataB;
    std::vector<uint8_t> vchENarr;
    opcodetype opCode;
    char cbuf[256];

    int32_t nOutputIdOuter = -1;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nOutputIdOuter++;
        // -- for each OP_RETURN need to check all other valid outputs

        // -- skip scan anon outputs
        if (tx.nVersion == ANON_TXN_VERSION
            && txout.IsAnonOutput())
            continue;

        CScript::const_iterator itTxA = txout.scriptPubKey.begin();

        if (!txout.scriptPubKey.GetOp(itTxA, opCode, vchEphemPK)
            || opCode != OP_RETURN)
            continue;
        else
        if (!txout.scriptPubKey.GetOp(itTxA, opCode, vchEphemPK)
            || vchEphemPK.size() != EC_COMPRESSED_SIZE)
        {
            // -- look for plaintext narrations
            if (vchEphemPK.size() > 1
                && vchEphemPK[0] == 'n'
                && vchEphemPK[1] == 'p')
            {
                if (txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && opCode == OP_RETURN
                    && txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && vchENarr.size() > 0)
                {
                    std::string sNarr = std::string(vchENarr.begin(), vchENarr.end());

                    snprintf(cbuf, sizeof(cbuf), "n_%d", nOutputIdOuter-1); // plaintext narration always matches preceding value output
                    mapNarr[cbuf] = sNarr;
                } else
                {
                    LogPrintf("%s Warning - tx: %s, Could not extract plaintext narration.\n", __func__, tx.GetHash().GetHex().c_str());
                };
            };
            continue;
        };

        int32_t nOutputId = -1;
        nStealth++;
        BOOST_FOREACH(const CTxOut& txoutB, tx.vout)
        {
            nOutputId++;

            // -- skip anon outputs
            if (tx.nVersion == ANON_TXN_VERSION
                && txout.IsAnonOutput())
                continue;

            if (&txoutB == &txout)
                continue;

            bool txnMatch = false; // only 1 txn will match an ephem pk

            CTxDestination address;
            if (!ExtractDestination(txoutB.scriptPubKey, address))
                continue;

            if (address.type() != typeid(CKeyID))
                continue;

            CKeyID ckidMatch = boost::get<CKeyID>(address);

            bool haveKey = HaveKey(ckidMatch); // if we allready have the key we still reprocess to store address mapping
            if (haveKey && fDebug)
                LogPrintf("Found existing stealth output key - txn has been processed before, reprocessing to store mapping.\n");

            std::set<CStealthAddress>::iterator it;
            for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
            {
                if (it->scan_secret.size() != EC_SECRET_SIZE)
                    continue; // stealth address is not owned

                memcpy(&sScan.e[0], &it->scan_secret[0], EC_SECRET_SIZE);

                if (StealthSecret(sScan, vchEphemPK, it->spend_pubkey, sShared, pkExtracted) != 0)
                {
                    LogPrintf("%s: StealthSecret failed.\n", __func__);
                    continue;
                };

                CPubKey cpkE(pkExtracted);

                if (!cpkE.IsValid())
                    continue;

                CKeyID ckidE = cpkE.GetID();

                if (ckidMatch != ckidE)
                    continue;

                if (fDebug)
                    LogPrintf("Found stealth txn to address %s\n", it->Encoded().c_str());

                if (haveKey) {
                    std::string sLabel = sStealthPrefix + it->Encoded().substr(0, 16) + "...";
                    SetAddressBookName(ckidE, sLabel);
                }
                else
                {
                    if (IsLocked())
                    {
                        if (fDebug)
                            LogPrintf("Wallet locked, adding key without secret.\n");

                        // -- add key without secret
                        std::vector<uint8_t> vchEmpty;
                        AddCryptedKey(cpkE, vchEmpty);
                        CKeyID keyId = cpkE.GetID();
                        CBitcoinAddress coinAddress(keyId);
                        std::string sLabel = sStealthPrefix + it->Encoded().substr(0, 16) + "...";
                        SetAddressBookName(keyId, sLabel);

                        CPubKey cpkEphem(vchEphemPK);
                        CPubKey cpkScan(it->scan_pubkey);
                        CStealthKeyMetadata lockedSkMeta(cpkEphem, cpkScan);

                        if (!CWalletDB(strWalletFile).WriteStealthKeyMeta(keyId, lockedSkMeta))
                            LogPrintf("WriteStealthKeyMeta failed for %s.\n", coinAddress.ToString().c_str());

                        mapStealthKeyMeta[keyId] = lockedSkMeta;
                        nFoundStealth++;
                    } else
                    {
                        if (it->spend_secret.size() != EC_SECRET_SIZE)
                            continue;

                        memcpy(&sSpend.e[0], &it->spend_secret[0], EC_SECRET_SIZE);

                        if (StealthSharedToSecretSpend(sShared, sSpend, sSpendR) != 0)
                        {
                            LogPrintf("StealthSharedToSecretSpend() failed.\n");
                            continue;
                        };

                        CKey ckey;
                        ckey.Set(&sSpendR.e[0], true);

                        if (!ckey.IsValid())
                        {
                            LogPrintf("%s: Reconstructed key is invalid.\n", __func__);
                            continue;
                        };

                        CPubKey cpkT = ckey.GetPubKey();
                        if (!cpkT.IsValid())
                        {
                            LogPrintf("%s: cpkT is invalid.\n", __func__);
                            continue;
                        };

                        CKeyID keyID = cpkT.GetID();

                        if (keyID != ckidMatch)
                        {
                            LogPrintf("%s: Spend key mismatch!\n", __func__);
                            continue;
                        };

                        if (fDebug)
                        {
                            CBitcoinAddress coinAddress(keyID);
                            LogPrintf("Adding key %s.\n", coinAddress.ToString().c_str());
                        };

                        if (!AddKeyPubKey(ckey, cpkT))
                        {
                            LogPrintf("%s: AddKeyPubKey failed.\n", __func__);
                            continue;
                        };


                        std::string sLabel = sStealthPrefix + it->Encoded().substr(0, 16) + "...";
                        SetAddressBookName(keyID, sLabel);
                        nFoundStealth++;
                    };
                }

                txnMatch = true;
                break;
            };

            if (txnMatch)
                break;

            // - ext account stealth keys
            ExtKeyAccountMap::const_iterator mi;
            for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
            {
                CExtKeyAccount *ea = mi->second;

                for (AccStealthKeyMap::iterator it = ea->mapStealthKeys.begin(); it != ea->mapStealthKeys.end(); ++it)
                {
                    const CEKAStealthKey &aks = it->second;

                    if (!aks.skScan.IsValid())
                        continue;

                    memcpy(&sScan.e[0], aks.skScan.begin(), EC_SECRET_SIZE);

                    if (StealthSecret(sScan, vchEphemPK, aks.pkSpend, sShared, pkExtracted) != 0)
                    {
                        LogPrintf("%s: StealthSecret failed.\n", __func__);
                        continue;
                    };

                    CPubKey cpkE(pkExtracted);

                    if (!cpkE.IsValid())
                        continue;
                    CKeyID ckidE = cpkE.GetID();

                    if (ckidMatch != ckidE)
                        continue;

                    if (haveKey) {
                        // - for compatability
                        std::string sLabel = sStealthPrefix + aks.ToStealthAddress().substr(0, 16) + "...";
                        SetAddressBookName(ckidMatch, sLabel);
                    }
                    else
                    {
                        if (fDebug)
                        {
                            LogPrintf("Found stealth txn to address %s\n", aks.ToStealthAddress().c_str());

                            // - check key if not locked
                            if (!IsLocked())
                            {
                                CKey kTest;

                                if (0 != ea->ExpandStealthChildKey(&aks, sShared, kTest))
                                {
                                    LogPrintf("%s: Error: ExpandStealthChildKey failed! %s.\n", __func__, aks.ToStealthAddress().c_str());
                                    continue;
                                };

                                CKeyID kTestId = kTest.GetPubKey().GetID();
                                if (kTestId != ckidMatch)
                                {
                                    LogPrintf("Error: Spend key mismatch!\n");
                                    continue;
                                };
                                CBitcoinAddress coinAddress(kTestId);
                                LogPrintf("Debug: ExpandStealthChildKey matches! %s, %s.\n", aks.ToStealthAddress().c_str(), coinAddress.ToString().c_str());
                            };

                        };

                        // - don't need to extract key now, wallet may be locked

                        CKeyID idStealthKey = aks.GetID();
                        CEKASCKey kNew(idStealthKey, sShared);
                        if (0 != ExtKeySaveKey(ea, ckidMatch, kNew))
                        {
                            LogPrintf("%s: Error: ExtKeySaveKey failed!\n", __func__);
                            continue;
                        };

                        // - for compatability
                        std::string sLabel = sStealthPrefix + aks.ToStealthAddress().substr(0, 16) + "...";
                        SetAddressBookName(ckidMatch, sLabel);
                    }

                    txnMatch = true;
                    break;
                };
                if (txnMatch)
                    break;
            };

            if (txnMatch)
            {
                // - process narration
                if (txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && opCode == OP_RETURN
                    && txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && vchENarr.size() > 0)
                {
                    SecMsgCrypter crypter;
                    crypter.SetKey(&sShared.e[0], &vchEphemPK[0]);
                    std::vector<uint8_t> vchNarr;
                    if (!crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr))
                    {
                        LogPrintf("%s: Decrypt narration failed.\n", __func__);
                        continue;
                    };
                    std::string sNarr = std::string(vchNarr.begin(), vchNarr.end());

                    snprintf(cbuf, sizeof(cbuf), "n_%d", nOutputId);
                    mapNarr[cbuf] = sNarr;
                };
                break;
            };
        };
    };

    return true;
};

static int IsAnonCoinCompromised(CTxDB *txdb, CPubKey &pubKey, CAnonOutput &ao, ec_point &vchSpentImage)
{
    // check if its been compromised (signer known)
    CKeyImageSpent kis;
    ec_point pkImage;
    bool fInMempool;

    getOldKeyImage(pubKey, pkImage);

    if (vchSpentImage == pkImage || GetKeyImage(txdb, pkImage, kis, fInMempool))
    {
        ao.nCompromised = 1;
        txdb->WriteAnonOutput(pubKey, ao);
        fStaleAnonCache = true; // force rebuild of anon cache
        LogPrintf("Spent key image, mark as compromised: %s\n", pubKey.GetID().ToString());
        return 1;
    }
    return 0;
}

bool CWallet::UndoAnonTransaction(const CTransaction& tx, const std::map<CKeyID, CStealthAddress> * const mapPubStealth, bool fEraseTx)
{
    if (fDebugRingSig)
        LogPrintf("UndoAnonTransaction() tx: %s\n", tx.GetHash().GetHex().c_str());
    // -- undo transaction - used if block is unlinked / txn didn't commit

    LOCK2(cs_main, cs_wallet);

    uint256 txnHash = tx.GetHash();

    CWalletDB walletdb(strWalletFile, "cr+");
    CTxDB txdb("cr+");

    // Remove all pub to stealth key mappings
    if (mapPubStealth != nullptr)
    {
        for (auto& element : *mapPubStealth) {
            DelAddressBookName(element.first, &walletdb);
        }
    }

    for (unsigned int i = 0; i < tx.vin.size(); ++i)
    {
        const CTxIn& txin = tx.vin[i];

        if (!txin.IsAnonInput())
            continue;

        ec_point vchImage;
        txin.ExtractKeyImage(vchImage);

        CKeyImageSpent spentKeyImage;

        bool fInMempool;
        if (!GetKeyImage(&txdb, vchImage, spentKeyImage, fInMempool))
        {
            if (fDebugRingSig)
                LogPrintf("Error: keyImage for input %d not found.\n", i);
            continue;
        };

        // Possible?
        if (spentKeyImage.txnHash != txnHash)
        {
            LogPrintf("Error: spentKeyImage for %s does not match txn %s.\n", HexStr(vchImage).c_str(), txnHash.ToString().c_str());
            continue;
        };

        if (!txdb.EraseKeyImage(vchImage))
        {
            LogPrintf("EraseKeyImage %d failed.\n", i);
            continue;
        };

        mapAnonOutputStats[spentKeyImage.nValue].decSpends(spentKeyImage.nValue);


        COwnedAnonOutput oao;
        if (walletdb.ReadOwnedAnonOutput(vchImage, oao))
        {
            if (fDebugRingSig)
                LogPrintf("UndoAnonTransaction(): input %d keyimage %s found in wallet (owned).\n", i, HexStr(vchImage).c_str());

            WalletTxMap::iterator mi = mapWallet.find(oao.outpoint.hash);
            if (mi == mapWallet.end())
            {
                LogPrintf("UndoAnonTransaction(): Error input %d prev txn not in mapwallet %s .\n", i, oao.outpoint.hash.ToString().c_str());
                return false;
            };

            CWalletTx& inTx = (*mi).second;
            if (oao.outpoint.n >= inTx.vout.size())
            {
                LogPrintf("UndoAnonTransaction(): bad wtx %s\n", oao.outpoint.hash.ToString().c_str());
                return false;
            } else
            if (inTx.IsSpent(oao.outpoint.n))
            {
                LogPrintf("UndoAnonTransaction(): found spent coin %s\n", oao.outpoint.hash.ToString().c_str());


                inTx.MarkUnspent(oao.outpoint.n);
                if (!walletdb.WriteTx(oao.outpoint.hash, inTx))
                {
                    LogPrintf("UndoAnonTransaction(): input %d WriteTx failed %s.\n", i, HexStr(vchImage).c_str());
                    return false;
                };
                inTx.MarkDirty(); // recalc balances
                NotifyTransactionChanged(this, oao.outpoint.hash, CT_UPDATED);
            };

            oao.fSpent = false;
            if (!walletdb.WriteOwnedAnonOutput(vchImage, oao))
            {
                LogPrintf("UndoAnonTransaction(): input %d WriteOwnedAnonOutput failed %s.\n", i, HexStr(vchImage).c_str());
                return false;
            };
        };
    };


    for (uint32_t i = 0; i < tx.vout.size(); ++i)
    {
        const CTxOut& txout = tx.vout[i];

        if (!txout.IsAnonOutput())
            continue;

        const CScript &s = txout.scriptPubKey;

        CPubKey pkCoin    = CPubKey(&s[2+1], EC_COMPRESSED_SIZE);
        CKeyID  ckCoinId  = pkCoin.GetID();

        CAnonOutput ao;
        if (!txdb.ReadAnonOutput(pkCoin, ao)) // read only to update mapAnonOutputStats
        {
            LogPrintf("ReadAnonOutput(): %u failed.\n", i);
            return false;
        };

        mapAnonOutputStats[ao.nValue].decExists(ao.nValue);
        mapAnonOutputStats[ao.nValue].nStakes -= ao.fCoinStake;

        if (!txdb.EraseAnonOutput(pkCoin))
        {
            LogPrintf("EraseAnonOutput(): %u failed.\n", i);
            continue;
        };

        // -- only in db if owned
        walletdb.EraseLockedAnonOutput(ckCoinId);

        std::vector<uint8_t> vchImage;

        if (!walletdb.ReadOwnedAnonOutputLink(pkCoin, vchImage))
        {
            if (fDebugRingSig)
                LogPrintf("ReadOwnedAnonOutputLink(): %u failed - output wasn't owned.\n", i);
            continue;
        };

        if (!walletdb.EraseOwnedAnonOutput(vchImage))
        {
            LogPrintf("EraseOwnedAnonOutput(): %u failed.\n", i);
            continue;
        };

        if (!walletdb.EraseOwnedAnonOutputLink(pkCoin))
        {
            LogPrintf("EraseOwnedAnonOutputLink(): %u failed.\n", i);
            continue;
        };
    };


    if (fEraseTx) // owned coinstake transaction are kept in the wallet
    {
        if (!walletdb.EraseTx(txnHash))
        {
            LogPrintf("UndoAnonTransaction() EraseTx %s failed.\n", txnHash.ToString().c_str());
            return false;
        }
        mapWallet.erase(txnHash);
    }

    return true;
};

bool CWallet::ProcessAnonTransaction(CWalletDB *pwdb, CTxDB *ptxdb, const CTransaction& tx, const uint256& blockHash, bool& fIsMine, mapValue_t& mapNarr, std::vector<WalletTxMap::iterator>& vUpdatedTxns, std::map<int64_t, CAnonBlockStat>& mapAnonBlockStat, const std::map<CKeyID, CStealthAddress> * const mapPubStealth)
{
    uint256 txnHash = tx.GetHash();

    if (fDebugRingSig)
    {
        LogPrintf("%s: tx: %s.\n", __func__, txnHash.GetHex().c_str());
        AssertLockHeld(cs_main);
        AssertLockHeld(cs_wallet);
    };

    // - txdb and walletdb must be in a transaction (no commit if fail)

    if (nNodeMode != NT_FULL)
    {
        return error("%s: Skipped - must run in full mode.\n", __func__);
    };

    int nBlockHeight = GetBlockHeightFromHash(blockHash);

    bool fHasNonAnonInputs = false;
    bool fHasNonAnonOutputs = false;
    bool fDebitAnonFromMe = false;
    for (uint32_t i = 0; i < tx.vin.size(); ++i)
    {
        const CTxIn& txin = tx.vin[i];

        if (!txin.IsAnonInput()) {
             fHasNonAnonInputs = true;
            continue;
        }

        const CScript &s = txin.scriptSig;

        ec_point vchImage;
        txin.ExtractKeyImage(vchImage);

        CKeyImageSpent spentKeyImage;

        bool fInMempool;
        if (GetKeyImage(ptxdb, vchImage, spentKeyImage, fInMempool))
        {
            if (spentKeyImage.txnHash == txnHash
                && spentKeyImage.inputNo == i)
            {
                if (fDebugRingSig)
                    LogPrintf("found matching spent key image - txn has been processed before -> reprocess vin[%d].\n", i);
            }
            else {
                if (TxnHashInSystem(ptxdb, spentKeyImage.txnHash))
                {
                    return error("%s: Error input %d keyimage %s already spent.", __func__, i, HexStr(vchImage).c_str());
                };

                if (fDebugRingSig)
                    LogPrintf("Input %d keyimage %s matches unknown txn %s, continuing.\n", i, HexStr(vchImage).c_str(), spentKeyImage.txnHash.ToString().c_str());

                // -- keyimage is in db, but invalid as does not point to a known transaction
                //    could be an old mempool keyimage
                //    continue
            }
        };


        COwnedAnonOutput oao;
        ec_point vchNewImage;
        if (!pwdb->ReadOldOutputLink(vchImage, vchNewImage))
            vchNewImage = vchImage;

        if (pwdb->ReadOwnedAnonOutput(vchNewImage, oao))
        {
            // remember that this transaction debits from me
            fDebitAnonFromMe = true;

            if (fDebugRingSig)
                LogPrintf("%s: input %d keyimage %s found in wallet (owned).\n", __func__, i, HexStr(vchImage).c_str());

            WalletTxMap::iterator mi = mapWallet.find(oao.outpoint.hash);
            if (mi == mapWallet.end())
                return error("%s: Error input %d prev txn not in mapwallet %s.", __func__, i, oao.outpoint.hash.ToString().c_str());

            CWalletTx& inTx = (*mi).second;
            if (oao.outpoint.n >= inTx.vout.size())
            {
                return error("%s: bad wtx %s.", __func__, oao.outpoint.hash.ToString().c_str());
            } else
            if (!inTx.IsSpent(oao.outpoint.n))
            {
                LogPrintf("%s: found spent coin %s.\n", __func__, oao.outpoint.hash.ToString().c_str());

                inTx.MarkSpent(oao.outpoint.n);
                if (!pwdb->WriteTx(oao.outpoint.hash, inTx))
                {
                    return error("%s: Input %d WriteTx failed %s.", __func__, i, HexStr(vchImage).c_str());
                };

                inTx.MarkDirty();           // recalc balances
                vUpdatedTxns.push_back(mi); // notify updates outside db txn
            };

            oao.fSpent = true;
            if (!pwdb->WriteOwnedAnonOutput(vchNewImage, oao))
            {
                return error("%s: Input %d WriteOwnedAnonOutput failed %s.", __func__, i, HexStr(vchImage).c_str());
            };
        }

        uint32_t nRingSize = (uint32_t)txin.ExtractRingSize();
        auto [nMinRingSize, nMaxRingSize] = GetRingSizeMinMax(tx.nTime);
        if (nRingSize < nMinRingSize || nRingSize > nMaxRingSize)
            return error("%s: Input %d ringsize %d not in range [%d, %d].", __func__, i, nRingSize, nMinRingSize, nMaxRingSize);

        const uint8_t *pPubkeys;
        int rsType;
        if (nRingSize > 1 && s.size() == 2 + EC_SECRET_SIZE + (EC_COMPRESSED_SIZE + EC_SECRET_SIZE) * nRingSize)
        {
            rsType = RING_SIG_2;
            pPubkeys = &s[2 + EC_SECRET_SIZE + EC_SECRET_SIZE * nRingSize];
        } else
        if (s.size() >= 2 + (EC_COMPRESSED_SIZE + EC_SECRET_SIZE + EC_SECRET_SIZE) * nRingSize)
        {
            rsType = RING_SIG_1;
            pPubkeys = &s[2];
        } else
            return error("%s: Input %d scriptSig too small.", __func__, i);

        int64_t nCoinValue = -1;

        CPubKey pkRingCoin;
        CAnonOutput ao;
        CTxIndex txindex;

        int minBlockHeight = tx.IsAnonCoinStake() ? Params().GetAnonStakeMinConfirmations() : MIN_ANON_SPEND_DEPTH;
        for (uint32_t ri = 0; ri < (uint32_t)nRingSize; ++ri)
        {
            pkRingCoin = CPubKey(&pPubkeys[ri * EC_COMPRESSED_SIZE], EC_COMPRESSED_SIZE);
            if (!ptxdb->ReadAnonOutput(pkRingCoin, ao))
                return error("%s: Input %u AnonOutput %s not found, rsType: %d.", __func__, i, HexStr(pkRingCoin).c_str(), rsType);

            if (IsAnonCoinCompromised(ptxdb, pkRingCoin, ao, vchImage) && Params().IsProtocolV3(nBestHeight))
                return error("%s: Found spent pubkey at index %u: AnonOutput: %s, rsType: %d.", __func__, i, HexStr(pkRingCoin).c_str(), rsType);

            if (nCoinValue == -1)
                nCoinValue = ao.nValue;
            else
            if (nCoinValue != ao.nValue)
                return error("%s: Input %u ring amount mismatch %d, %d.", __func__, i, nCoinValue, ao.nValue);

            if (ao.nBlockHeight == 0
                || nBestHeight - ao.nBlockHeight + 1 < minBlockHeight) // ao confirmed in last block has depth of 1
                return error("%s: Input %u ring coin %u depth < %d.", __func__, i, ri, minBlockHeight);

            if (blockHash != 0 && nRingSize == 1)
            {
                ao.nCompromised = 1;
                if (!ptxdb->WriteAnonOutput(pkRingCoin, ao))
                    return error("%s: Input %d WriteAnonOutput failed %s.", __func__, i, HexStr(vchImage).c_str());
                fStaleAnonCache = true; // force rebuild of anon cache
            }

            // -- ring sig validation is done in CTransaction::CheckAnonInputs()
        }

        spentKeyImage.txnHash = txnHash;
        spentKeyImage.inputNo = i;
        spentKeyImage.nValue  = nCoinValue;
        spentKeyImage.nBlockHeight = nBlockHeight;

        mapAnonBlockStat[nCoinValue].nSpends++;

        if (blockHash != 0)
        {
            if (!ptxdb->WriteKeyImage(vchImage, spentKeyImage))
                return error("%s: Input %d WriteKeyImage failed %s.", __func__, i, HexStr(vchImage).c_str());
        } else
            // -- add keyImage to mempool, will be added to txdb in UpdateAnonTransaction
            mempool.insertKeyImage(vchImage, spentKeyImage);
    }

    ec_secret sSpendR;
    ec_secret sSpend;
    ec_secret sScan;
    ec_secret sShared;

    ec_point pkExtracted;

    std::vector<uint8_t> vchEphemPK;
    std::vector<uint8_t> vchDataB;
    std::vector<uint8_t> vchENarr;

    std::vector<std::vector<uint8_t> > vPrevMatch;
    char cbuf[256];

    try { vchEphemPK.resize(EC_COMPRESSED_SIZE); } catch (std::exception& e)
    {
        return error("%s: vchEphemPK.resize threw: %s.", __func__, e.what());
    };

    std::map<CKeyID, std::string> mapOutReceiveAddr;
    bool fNotAllOutputsOwned = false;
    for (uint32_t i = 0; i < tx.vout.size(); ++i)
    {
        const CTxOut& txout = tx.vout[i];

        if (!txout.IsAnonOutput()) {
            fHasNonAnonOutputs = true;
            continue;
        }

        const CScript &s = txout.scriptPubKey;

        CPubKey pkCoin   = CPubKey(&s[2+1], EC_COMPRESSED_SIZE);
        CKeyID  ckCoinId = pkCoin.GetID();

        COutPoint outpoint = COutPoint(tx.GetHash(), i);

        // -- add all anon outputs to txdb
        CAnonOutput ao;

        if (ptxdb->ReadAnonOutput(pkCoin, ao)) // check if exists
        {
            if (blockHash != 0)
            {
                if (fDebugRingSig)
                    LogPrintf("Found existing anon output - assuming txn has been processed before -> reprocess vout[%d].\n", i);

                if (ao.nBlockHeight && ao.nBlockHeight != nBlockHeight)
                {
                    LogPrintf("%s: Warning: persisted block height of anon %s does not match current block %s -> ATXO cache must be rebuild.\n", __func__, ao.nBlockHeight, nBlockHeight);
                    fStaleAnonCache = true; // force rebuild of anon cache
                }
                ao.nBlockHeight = nBlockHeight;
                ao.fCoinStake = tx.IsCoinStake();
            }
            else {
                return error("%s: Found duplicate anon output.", __func__);
            }
        }
        else {
            ao = CAnonOutput(outpoint, txout.nValue, nBlockHeight, 0, tx.IsCoinStake());         
        }
        if (!ptxdb->WriteAnonOutput(pkCoin, ao))
        {
            LogPrintf("%s: WriteAnonOutput failed.\n", __func__);
            continue;
        };
        // add to anon cache
        if (tx.IsAnonCoinStake())
            mapAnonBlockStat[txout.nValue].nStakingOutputs++;
        else
            mapAnonBlockStat[txout.nValue].nOutputs++;

        memcpy(&vchEphemPK[0], &s[2+EC_COMPRESSED_SIZE+2], EC_COMPRESSED_SIZE);

        bool fHaveSpendKey = false;
        bool fOwnOutput = false;
        CPubKey cpkE;
        data_chunk pkScan;
        std::string sSxAddr;
        for (std::set<CStealthAddress>::iterator it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
        {
            if (it->scan_secret.size() != EC_SECRET_SIZE)
                continue; // stealth address is not owned

            memcpy(&sScan.e[0], &it->scan_secret[0], EC_SECRET_SIZE);

            if (StealthSecret(sScan, vchEphemPK, it->spend_pubkey, sShared, pkExtracted) != 0)
            {
                LogPrintf("StealthSecret failed.\n");
                continue;
            };

            cpkE = CPubKey(pkExtracted);

            if (!cpkE.IsValid()
                || cpkE != pkCoin)
                continue;

            pkScan = it->scan_pubkey;
            sSxAddr = it->Encoded();

            if (!IsLocked())
            {
                if (it->spend_secret.size() != EC_SECRET_SIZE)
                {
                    LogPrintf("%s: Found anon tx to sx key %s which contains no spend secret.\n", __func__, sSxAddr.c_str());
                    continue;
                    // - next iter here, stop processing (fOwnOutput not set)
                };
                memcpy(&sSpend.e[0], &it->spend_secret[0], EC_SECRET_SIZE);

                if (StealthSharedToSecretSpend(sShared, sSpend, sSpendR) != 0)
                {
                    LogPrintf("%s: StealthSharedToSecretSpend() failed.\n", __func__);
                    continue;
                };

                fHaveSpendKey = true;
            };

            fOwnOutput = true;
            break;
        };

        // - check ext account stealth keys
        ExtKeyAccountMap::const_iterator mi;
        if (!fOwnOutput)
        for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
        {
            CExtKeyAccount *ea = mi->second;

            for (AccStealthKeyMap::iterator it = ea->mapStealthKeys.begin(); it != ea->mapStealthKeys.end(); ++it)
            {
                const CEKAStealthKey &aks = it->second;

                if (!aks.skScan.IsValid())
                    continue;

                memcpy(&sScan.e[0], aks.skScan.begin(), EC_SECRET_SIZE);
                if (StealthSecret(sScan, vchEphemPK, aks.pkSpend, sShared, pkExtracted) != 0)
                {
                    LogPrintf("%s: StealthSecret failed.\n", __func__);
                    continue;
                };

                CPubKey cpkE(pkExtracted);
                if (!cpkE.IsValid()
                    || cpkE != pkCoin)
                    continue;

                pkScan = aks.pkScan;
                sSxAddr = aks.ToStealthAddress();

                if (!ea->IsLocked(aks))
                {
                    CKey kChild;

                    if (0 != ea->ExpandStealthChildKey(&aks, sShared, kChild))
                    {
                        LogPrintf("%s: ExpandStealthChildKey failed.\n", __func__);
                        // - carry on, account could be crypted separately
                    } else
                    {
                        memcpy(&sSpendR.e[0], kChild.begin(), EC_SECRET_SIZE);
                        fHaveSpendKey = true;
                    };
                } else
                {
                    if (fDebug)
                        LogPrintf("Chain %d of %s IsLocked.\n", aks.akSpend.nParent, ea->GetIDString58());
                };

                fOwnOutput = true;
                break;
            };
            if (fOwnOutput)
                break;
        };

        if (!fOwnOutput) {
            if (mapPubStealth && mapPubStealth->count(ckCoinId)) {
                // if we have stealth address for the non owned pubkey, add the mapping to the addressbook
                SetAddressBookName(ckCoinId, mapPubStealth->at(ckCoinId).Encoded(), pwdb, false);
            }
            fNotAllOutputsOwned = true; // remember that at least one output is not owned
            continue;
        }

        if (fDebugRingSig)
            LogPrintf("anon output match tx, no %s, %u\n", txnHash.GetHex().c_str(), i);

        fIsMine = true; // mark tx to be added to wallet

        int lenENarr = 0;
        if (s.size() > MIN_ANON_OUT_SIZE)
            lenENarr = s[2+EC_COMPRESSED_SIZE+1 + EC_COMPRESSED_SIZE+1];

        if (lenENarr > 0)
        {
            if (fDebugRingSig)
                LogPrintf("Processing encrypted narration of %d bytes\n", lenENarr);

            try { vchENarr.resize(lenENarr); } catch (std::exception& e)
            {
                LogPrintf("%s: Error: vchENarr.resize threw: %s.\n", __func__, e.what());
                continue;
            };

            memcpy(&vchENarr[0], &s[2+EC_COMPRESSED_SIZE+1+EC_COMPRESSED_SIZE+2], lenENarr);

            SecMsgCrypter crypter;
            crypter.SetKey(&sShared.e[0], &vchEphemPK[0]);
            std::vector<uint8_t> vchNarr;
            if (!crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr))
            {
                LogPrintf("%s: Decrypt narration failed.\n", __func__);
                continue;
            };
            std::string sNarr = std::string(vchNarr.begin(), vchNarr.end());

            snprintf(cbuf, sizeof(cbuf), "n_%u", i);
            mapNarr[cbuf] = sNarr;
        };

        if (!fHaveSpendKey)
        {
            std::vector<uint8_t> vchEmpty;
            CWalletDB *pwalletdbEncryptionOld = pwalletdbEncryption;
            pwalletdbEncryption = pwdb; // HACK, pass pdb to AddCryptedKey
            AddCryptedKey(pkCoin, vchEmpty);
            pwalletdbEncryption = pwalletdbEncryptionOld;

            if (fDebugRingSig)
                LogPrintf("Wallet locked, adding key without secret.\n");

            CPubKey cpkEphem(vchEphemPK);
            CPubKey cpkScan(pkScan);
            CLockedAnonOutput lockedAo(cpkEphem, cpkScan, COutPoint(txnHash, i));
            if (!pwdb->WriteLockedAnonOutput(ckCoinId, lockedAo))
            {
                CBitcoinAddress coinAddress(ckCoinId);
                LogPrintf("%s: WriteLockedAnonOutput failed for %s.\n", __func__, coinAddress.ToString().c_str());
            };
        } else
        {
            ec_point pkTestSpendR;
            if (SecretToPublicKey(sSpendR, pkTestSpendR) != 0)
            {
                LogPrintf("%s: SecretToPublicKey() failed.\n", __func__);
                continue;
            };

            CKey ckey;
            ckey.Set(&sSpendR.e[0], true);

            if (!ckey.IsValid())
            {
                LogPrintf("%s: Reconstructed key is invalid.\n", __func__);
                continue;
            };

            CPubKey cpkT = ckey.GetPubKey();

            if (!cpkT.IsValid()
                || cpkT != pkCoin)
            {
                LogPrintf("%s: cpkT is invalid.\n", __func__);
                continue;
            };

            if (fDebugRingSig)
            {
                CBitcoinAddress coinAddress(ckCoinId);
                LogPrintf("Adding key %s.\n", coinAddress.ToString().c_str());
            };

            if (!AddKeyInDBTxn(pwdb, ckey))
            {
                LogPrintf("%s: AddKeyInDBTxn failed.\n", __func__);
                continue;
            };

            // -- store keyImage
            ec_point pkImage;
            ec_point pkOldImage;
            getOldKeyImage(pkCoin, pkOldImage);
            if (generateKeyImage(pkTestSpendR, sSpendR, pkImage) != 0)
            {
                LogPrintf("%s: generateKeyImage() failed.\n", __func__);
                continue;
            }

            CKeyImageSpent kis;
            bool fInMemPool;
            bool fSpentAOut = false;
            // shouldn't be possible for kis to be in mempool here
            fSpentAOut = (GetKeyImage(ptxdb, pkImage, kis, fInMemPool)
                        ||GetKeyImage(ptxdb, pkOldImage, kis, fInMemPool));

            COwnedAnonOutput oao(outpoint, fSpentAOut);

            if (!pwdb->WriteOwnedAnonOutput(pkImage, oao)
              ||!pwdb->WriteOldOutputLink(pkOldImage, pkImage)
              ||!pwdb->WriteOwnedAnonOutputLink(pkCoin, pkImage))
            {
                LogPrintf("%s: WriteOwnedAnonOutput() failed.\n", __func__);
                continue;
            };

            if (fDebugRingSig)
                LogPrintf("Adding anon output to wallet: %s.\n", HexStr(pkImage).c_str());
        };

        // Remember used stealth address
        std::string sLabel = sAnonPrefix + sSxAddr.substr(0, 16) + "...";
        mapOutReceiveAddr[ckCoinId] = sLabel;
    };

    if (!mapOutReceiveAddr.empty())
    {
        // detect non change anon outputs an add them to te addressbook
        for (auto const& out : mapOutReceiveAddr)
        {
            // if nonAnonInputs exists, anonOutputs are never change
            if (fHasNonAnonInputs ||
                    // AnonCoinStake outputs are never change
                    tx.IsAnonCoinStake() || (
                        // if nonAnonOutputs exists, anonOutputs are change when anon is debited from us
                        !(fDebitAnonFromMe && fHasNonAnonOutputs) &&
                        // if not all outputs are owned, owned anonOutputs are change when anon is debited from us
                        !(fDebitAnonFromMe && fNotAllOutputsOwned) ))
            {
                SetAddressBookName(out.first, out.second, pwdb, false);
            }
            else {
                // don't add change outputs to the addressbook
                // legacy: remove change outputs added from previous wallet versions
                DelAddressBookName(out.first, pwdb);
            }
        }
    }


    return true;
};

bool CWallet::GetAnonChangeAddress(CStealthAddress &sxAddress)
{
    // return owned stealth address to send anon change to.
    // TODO: make an option

    // NOTE: tries default ext account only, for now
    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idDefaultAccount);
    if (mi != mapExtAccounts.end())
    {
        CExtKeyAccount *ea = mi->second;

        AccStealthKeyMap::iterator it = ea->mapStealthKeys.begin();

        if (it != ea->mapStealthKeys.end())
            return (0 == it->second.SetSxAddr(sxAddress));
    };

    std::set<CStealthAddress>::iterator it;
    for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() < 1)
            continue; // stealth address is not owned

        sxAddress = *it;
        return true;
    };

    return false;
};

bool CWallet::GetAnonStakeAddress(const COwnedAnonOutput& stakedOao, CStealthAddress& sxAddress)
{
    // TODO Get corresponding stealth address for stakedOao
    LOCK(cs_wallet);

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idDefaultAccount);
    if (mi != mapExtAccounts.end())
    {
        CExtKeyAccount *ea = mi->second;

        AccStealthKeyMap::iterator it = ea->mapStealthKeys.begin();

        if (it != ea->mapStealthKeys.end())
        {
            if (0 == it->second.SetSxAddr(sxAddress))
            {
                CKey skSpend;
                if (ea->GetKey(it->second.akSpend, skSpend))
                {
                    sxAddress.spend_secret.resize(EC_SECRET_SIZE);
                    memcpy(&sxAddress.spend_secret[0], skSpend.begin(), EC_SECRET_SIZE);
                    return true;
                }
            }
            return false;
        }
    };

    std::set<CStealthAddress>::iterator it;
    for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() < 1)
            continue; // stealth address is not owned

        sxAddress = *it;
        return true;
    };
    return false;
}


bool CWallet::CreateStealthOutput(CStealthAddress* sxAddress, int64_t nValue, std::string& sNarr, std::vector<std::pair<CScript, int64_t> >& vecSend, CScript& scriptNarration, std::string& sError)
{
    if (fDebugRingSig)
        LogPrintf("CreateStealthOutput()\n");

    if (!sxAddress)
    {
        sError = "!sxAddress, todo.";
        return false;
    };

    ec_secret ephem_secret;
    ec_secret secretShared;
    ec_point pkSendTo;
    ec_point ephem_pubkey;

    if (GenerateRandomSecret(ephem_secret) != 0)
    {
        sError = "GenerateRandomSecret failed.";
        return false;
    };

    if (StealthSecret(ephem_secret, sxAddress->scan_pubkey, sxAddress->spend_pubkey, secretShared, pkSendTo) != 0)
    {
        sError = "Could not generate receiving public key.";
        return false;
    };

    CPubKey cpkTo(pkSendTo);
    if (!cpkTo.IsValid())
    {
        sError = "Invalid public key generated.";
        return false;
    };

    CKeyID ckidTo = cpkTo.GetID();

    CBitcoinAddress addrTo(ckidTo);

    if (SecretToPublicKey(ephem_secret, ephem_pubkey) != 0)
    {
        sError = "Could not generate ephem public key.";
        return false;
    };

    if (fDebug)
    {
        LogPrintf("CreateStealthOutput() to generated pubkey %u: %s\n", pkSendTo.size(), HexStr(pkSendTo).c_str());
        LogPrintf("hash %s\n", addrTo.ToString().c_str());
        LogPrintf("ephem_pubkey %u: %s\n", ephem_pubkey.size(), HexStr(ephem_pubkey).c_str());
    };

    std::vector<unsigned char> vchENarr;
    if (sNarr.length() > 0)
    {
        SecMsgCrypter crypter;
        crypter.SetKey(&secretShared.e[0], &ephem_pubkey[0]);

        if (!crypter.Encrypt((uint8_t*)&sNarr[0], sNarr.length(), vchENarr))
        {
            sError = "Narration encryption failed.";
            return false;
        };

        if (vchENarr.size() > MAX_STEALTH_NARRATION_SIZE)
        {
            sError = "Encrypted narration is too long.";
            return false;
        };
    };


    CScript scriptPubKey;
    scriptPubKey.SetDestination(addrTo.Get());

    vecSend.push_back(make_pair(scriptPubKey, nValue));

    CScript scriptP = CScript() << OP_RETURN << ephem_pubkey;
    if (vchENarr.size() > 0) {
        scriptP = scriptP << OP_RETURN << vchENarr;
        scriptNarration = scriptP;
    }

    vecSend.push_back(make_pair(scriptP, 0));

    return true;
};

bool CWallet::CreateAnonOutputs(CStealthAddress* sxAddress, int64_t nValue, std::string& sNarr, std::vector<std::pair<CScript, int64_t> >& vecSend, CScript& scriptNarration, std::map<CKeyID, CStealthAddress>* const mapPubStealth, std::vector<ec_secret> * const vecSecShared, int64_t maxAnonOutput)
{
    std::vector<int64_t> vOutAmounts;
    if (splitAmount(nValue, vOutAmounts, maxAnonOutput) != 0)
    {
        LogPrintf("splitAmount() failed.\n");
        return false;
    };
    return CreateAnonOutputs(sxAddress, vOutAmounts, sNarr, vecSend, scriptNarration, mapPubStealth, vecSecShared);
}

bool CWallet::CreateAnonOutputs(CStealthAddress* sxAddress, std::vector<int64_t>& vOutAmounts, std::string& sNarr, std::vector<std::pair<CScript, int64_t> >& vecSend, CScript& scriptNarration, std::map<CKeyID, CStealthAddress>* const mapPubStealth, std::vector<ec_secret> * const vecSecShared)
{
    if (fDebugRingSig)
        LogPrintf("CreateAnonOutputs()\n");

    ec_secret scEphem;
    ec_point  pkSendTo;
    ec_point  pkEphem;

    // -- output scripts OP_RETURN ANON_TOKEN pkTo R enarr
    //    Each outputs split from the amount must go to a unique pk, or the key image would be the same
    //    Only the first output of the group carries the enarr (if present)  
    for (uint32_t i = 0; i < vOutAmounts.size(); ++i)
    {
        ec_secret scShared;
        CPubKey   cpkTo;

        if (GenerateRandomSecret(scEphem) != 0)
        {
            LogPrintf("GenerateRandomSecret failed.\n");
            return false;
        };

        if (sxAddress) // NULL for test only
        {
            if (StealthSecret(scEphem, sxAddress->scan_pubkey, sxAddress->spend_pubkey, scShared, pkSendTo) != 0)
            {
                LogPrintf("Could not generate receiving public key.\n");
                return false;
            };

            cpkTo = CPubKey(pkSendTo);
            if (!cpkTo.IsValid())
            {
                LogPrintf("Invalid public key generated.\n");
                return false;
            };

            if (SecretToPublicKey(scEphem, pkEphem) != 0)
            {
                LogPrintf("Could not generate ephem public key.\n");
                return false;
            };

            if (mapPubStealth)
                // save which stealth address was used for creating this key
                (*mapPubStealth)[cpkTo.GetID()] = *sxAddress;
        };

        CScript scriptSendTo;
        scriptSendTo.push_back(OP_RETURN);
        scriptSendTo.push_back(OP_ANON_MARKER);
        scriptSendTo << cpkTo;
        scriptSendTo << pkEphem;

        if (i == 0 && sNarr.length() > 0)
        {
            std::vector<unsigned char> vchNarr;
            SecMsgCrypter crypter;
            crypter.SetKey(&scShared.e[0], &pkEphem[0]);

            if (!crypter.Encrypt((uint8_t*)&sNarr[0], sNarr.length(), vchNarr))
            {
                LogPrintf("Narration encryption failed.\n");
                return false;
            };

            if (vchNarr.size() > MAX_STEALTH_NARRATION_SIZE)
            {
                LogPrintf("Encrypted narration is too long.\n");
                return false;
            };
            scriptSendTo << vchNarr;
            scriptNarration = scriptSendTo;
        };

        if (fDebug)
        {
            CKeyID ckidTo = cpkTo.GetID();
            CBitcoinAddress addrTo(ckidTo);

            LogPrintf("CreateAnonOutput to generated pubkey %u: %s\n", pkSendTo.size(), HexStr(pkSendTo).c_str());
            if (!sxAddress)
                LogPrintf("Test Mode\n");
            LogPrintf("hash %s\n", addrTo.ToString().c_str());
            LogPrintf("ephemeral pubkey %u: %s\n", pkEphem.size(), HexStr(pkEphem).c_str());

            LogPrintf("scriptPubKey %s\n", scriptSendTo.ToString().c_str());
        };
        vecSend.push_back(make_pair(scriptSendTo, vOutAmounts[i]));
        if (vecSecShared)
             // save which shared secret was used for creating this key
            vecSecShared->push_back(scShared);
    };

    return true;
};

static bool checkCombinations(int64_t nReq, int m, std::vector<COwnedAnonOutput*>& vData, std::vector<int>& vecInputIndex)
{
    // -- m of n combinations, check smallest coins first

    if (fDebugRingSig)
        LogPrintf("checkCombinations() %d, %u\n", m, vData.size());

    int nOwnedAnonOutputs = vData.size();

    try { vecInputIndex.resize(m); } catch (std::exception& e)
    {
        LogPrintf("Error: checkCombinations() v.resize(%d) threw: %s.\n", m, e.what());
        return false;
    };


    int64_t nCount = 0;

    if (m > nOwnedAnonOutputs) // ERROR
    {
        LogPrintf("Error: checkCombinations() m > nOwnedAnonOutputs\n");
        return false;
    };

    int i, l, startL = 0;

    // -- pick better start point
    //    lAvailableCoins is sorted, if coin i * m < nReq, no combinations of lesser coins will be < either
    for (l = m; l <= nOwnedAnonOutputs; ++l)
    {
        if (vData[l-1]->nValue * m < nReq)
            continue;
        startL = l;
        break;
    };

    if (fDebugRingSig)
        LogPrintf("Starting at level %d\n", startL);

    if (startL == 0)
    {
        if (fDebugRingSig)
            LogPrintf("checkCombinations() No possible combinations.\n");
        return false;
    };


    for (l = startL; l <= nOwnedAnonOutputs; ++l)
    {
        for (i = 0; i < m; ++i)
            vecInputIndex[i] = (l - i)-1;

        // -- m must be > 2 to use coarse seeking
        bool fSeekFine = m <= 2;

        if(fDebugRingSig)
            LogPrintf("coarse seek: %d, vecInputIndex[1]: %d, vecInputIndex[0]-1: %d\n", !fSeekFine, vecInputIndex[1], vecInputIndex[0]-1);
        // -- coarse
        while(!fSeekFine && vecInputIndex[1] < vecInputIndex[0]-1)
        {
            for (i = 1; i < m; ++i)
                vecInputIndex[i] = vecInputIndex[i]+1;

            int64_t nTotal = 0;

            for (i = 0; i < m; ++i)
                nTotal += vData[vecInputIndex[i]]->nValue;

            if(fDebugRingSig)
                LogPrintf("coarse seeking - nTotal: %d\n", nTotal);

            nCount++;

            if (nTotal == nReq)
            {
                if (fDebugRingSig)
                {
                    LogPrintf("Found match of total %d, in %d tries, ", nTotal, nCount);
                    for (i = m; i--;) LogPrintf("%d%c", vecInputIndex[i], i ? ' ': '\n');
                };
                return true;
            };
            if (nTotal > nReq)
            {
                for (i = 1; i < m; ++i) // rewind
                    vecInputIndex[i] = vecInputIndex[i]-1;

                if (fDebugRingSig)
                {
                    LogPrintf("Found coarse match of total %d, in %d tries\n", nTotal, nCount);
                    for (i = m; i--;) LogPrintf("%d%c", vecInputIndex[i], i ? ' ': '\n');
                };
                fSeekFine = true;
            };
        };

        if (!fSeekFine && l < nOwnedAnonOutputs)
            continue;

        // -- fine
        i = m-1;
        for (;;)
        {
            if (vecInputIndex[0] == l-1) // otherwise get duplicate combinations
            {
                int64_t nTotal = 0;

                for (i = 0; i < m; ++i)
                    nTotal += vData[vecInputIndex[i]]->nValue;

                if(fDebugRingSig)
                    LogPrintf("fine seeking - nTotal: %d\n", nTotal);

                nCount++;

                if (nTotal >= nReq)
                {
                    if (fDebugRingSig)
                    {
                        LogPrintf("Found match of total %d, in %d tries\n", nTotal, nCount);
                        for (i = m; i--;) LogPrintf("%d%c", vecInputIndex[i], i ? ' ': '\n');
                    };
                    return true;
                };

                if (fDebugRingSig && !(nCount % 500))
                {
                    LogPrintf("checkCombinations() nCount: %d - l: %d, nOwnedAnonOutputs: %d, m: %d, i: %d, nReq: %d, v[0]: %d, nTotal: %d \n", nCount, l, nOwnedAnonOutputs, m, i, nReq, vecInputIndex[0], nTotal);
                    for (i = m; i--;) LogPrintf("%d%c", vecInputIndex[i], i ? ' ': '\n');
                };
            };

            for (i = 0; vecInputIndex[i] >= l - i;) // 0 is largest element
            {
                if (++i >= m)
                    goto EndInner;
            };

            // -- fill the set with the next values
            for (vecInputIndex[i]++; i; i--)
                vecInputIndex[i-1] = vecInputIndex[i] + 1;
        };
        EndInner:
        if (i+1 > nOwnedAnonOutputs)
            break;
    };

    return false;
}

int CWallet::PickAnonInputs(int rsType, int64_t nValue, int64_t& nFee, int nRingSize, CWalletTx& wtxNew, int nOutputs, int nSizeOutputs, int& nExpectChangeOuts, std::list<COwnedAnonOutput>& lAvailableCoins, std::vector<const COwnedAnonOutput*>& vPickedCoins, std::vector<std::pair<CScript, int64_t> >& vecChange, bool fTest, std::string& sError, int feeMode)
{
    if (fDebugRingSig)
        LogPrintf("PickAnonInputs(), ChangeOuts %d, FeeMode %d\n", nExpectChangeOuts, feeMode);
    // - choose the smallest coin that can cover the amount (feeMode==1) or the amount + fee (feeMode!=1)
    //   or least no. of smallest coins


    int64_t nAmountCheck = 0;

    std::vector<COwnedAnonOutput*> vData;
    try { vData.resize(lAvailableCoins.size()); } catch (std::exception& e)
    {
        LogPrintf("Error: PickAnonInputs() vData.resize threw: %s.\n", e.what());
        return false;
    };

    uint32_t vi = 0;
    for (std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin(); it != lAvailableCoins.end(); ++it)
    {
        vData[vi++] = &(*it);
        nAmountCheck += it->nValue;
    };

    uint32_t nByteSizePerInCoin;
    switch(rsType)
    {
        case RING_SIG_1:
            nByteSizePerInCoin = (sizeof(COutPoint) + sizeof(unsigned int)) // CTxIn
                + GetSizeOfCompactSize(2 + (33 + 32 + 32) * nRingSize)
                + 2 + (33 + 32 + 32) * nRingSize;
            break;
        case RING_SIG_2:
            nByteSizePerInCoin = (sizeof(COutPoint) + sizeof(unsigned int)) // CTxIn
                + GetSizeOfCompactSize(2 + 32 + (33 + 32) * nRingSize)
                + 2 + 32 + (33 + 32) * nRingSize;
            break;
        default:
            sError = "Unknown ring signature type.";
            return false;
    };

    if (fDebugRingSig)
        LogPrintf("nByteSizePerInCoin: %d\n", nByteSizePerInCoin);

    // -- repeat until all levels are tried (1 coin, 2 coins, 3 coins etc)
    for (uint32_t i = 0; i < lAvailableCoins.size(); ++i)
    {
        if (fDebugRingSig)
            LogPrintf("Input loop %u\n", i);

        uint32_t nTotalBytes = (4 + 4 + 4) // Ctx: nVersion, nTime, nLockTime
            + GetSizeOfCompactSize(nOutputs + nExpectChangeOuts)
            + nSizeOutputs
            + (GetSizeOfCompactSize(MIN_ANON_OUT_SIZE) + MIN_ANON_OUT_SIZE + sizeof(int64_t)) * nExpectChangeOuts
            + GetSizeOfCompactSize((i+1))
            + nByteSizePerInCoin * (i+1);

        nFee = wtxNew.GetMinFee(0, GMF_SEND, nTotalBytes);
        if (nFee == MAX_MONEY)
        {
            sError = "The transaction is over the maximum size limit. Create multiple transactions with smaller amounts.";
            if (fDebugRingSig)
                LogPrintf("Transaction with nTotalBytes %d results in MAX_MONEY fee.\n", nTotalBytes);
            return 3;
        }

        int64_t nValueTest;
        if (feeMode == 1) {
            nValueTest = nValue;
        }
        else {
            nValueTest = nValue + nFee;

            int64_t nFeeDiff = nAmountCheck - nValueTest;
            if (nFeeDiff < 0)
            {
                // substract fee
                nValueTest += nFeeDiff;
                if (nValueTest <= 0)
                {
                    sError = "Not enough (mature) coins with requested ring size to cover amount with fees.";
                    if (fDebugRingSig)
                        LogPrintf("Not enough (mature) coins %d with requested ring size to cover amount %d together with fees %d.\n", nAmountCheck, nValue, nFee);
                    return 3;
                }

                if (fDebugRingSig)
                    LogPrintf("AmountWithFeeExceedsBalance! simulate exhaustive trx, lower amount by nFeeDiff: %d\n", nFeeDiff);

                if (nExpectChangeOuts != 0) {
                    // -- set nExpectChangeOuts to 0 to simulate exhaustive payment (total+fee=totalAvailableCoins)
                    nExpectChangeOuts = 0;
                    // -- get nTotalBytes again for 0 change outputs
                    uint32_t nTotalBytes = (4 + 4 + 4) // Ctx: nVersion, nTime, nLockTime
                        + GetSizeOfCompactSize(nOutputs)
                        + nSizeOutputs
                        + (GetSizeOfCompactSize(MIN_ANON_OUT_SIZE) + MIN_ANON_OUT_SIZE + sizeof(int64_t)) * vecChange.size()
                        + GetSizeOfCompactSize((i + 1))
                        + nByteSizePerInCoin * (i + 1);

                    if (fDebugRingSig)
                        LogPrintf("New nTotalBytes: %d\n", nTotalBytes);
                }
            }
        }

        if (fDebugRingSig)
            LogPrintf("nValue: %d, nFee: %d, nValueTest: %d, nAmountCheck: %d, nTotalBytes: %u\n", nValue, nFee, nValueTest, nAmountCheck, nTotalBytes);

        vPickedCoins.clear();
        vecChange.clear();

        std::vector<int> vecInputIndex;

        if (checkCombinations(nValueTest, i+1, vData, vecInputIndex))
        {
            if (fDebugRingSig)
            {
                LogPrintf("Found combination %u, ", i+1);
                for (int ic = vecInputIndex.size(); ic--;)
                    LogPrintf("%d%c", vecInputIndex[ic], ic ? ' ': '\n');

                LogPrintf("nTotalBytes %u\n", nTotalBytes);
                LogPrintf("nFee %d\n", nFee);
            };

            int64_t nTotalIn = 0;
            vPickedCoins.resize(vecInputIndex.size());
            for (uint32_t ic = 0; ic < vecInputIndex.size(); ++ic)
            {
                vPickedCoins[ic] = vData[vecInputIndex[ic]];
                nTotalIn += vPickedCoins[ic]->nValue;
            };

            int64_t nChange = nTotalIn - nValueTest;


            CStealthAddress sxChange;
            if (!GetAnonChangeAddress(sxChange))
            {
                sError = "GetAnonChangeAddress() change failed.";
                return 3;
            };

            std::string sNone;
            sNone.clear();
            CScript scriptNone;
            if (!CreateAnonOutputs(fTest ? NULL : &sxChange, nChange, sNone, vecChange, scriptNone))
            {
                sError = "CreateAnonOutputs() change failed.";
                return 3;
            };


            // -- get nTotalBytes again, using actual no. of change outputs
            uint32_t nTotalBytes = (4 + 4 + 4) // Ctx: nVersion, nTime, nLockTime
                + GetSizeOfCompactSize(nOutputs + vecChange.size())
                + nSizeOutputs
                + (GetSizeOfCompactSize(MIN_ANON_OUT_SIZE) + MIN_ANON_OUT_SIZE + sizeof(int64_t)) * vecChange.size()
                + GetSizeOfCompactSize((i+1))
                + nByteSizePerInCoin * (i+1);

            int64_t nTestFee = wtxNew.GetMinFee(0, GMF_SEND, nTotalBytes);

            if (nTestFee > nFee)
            {
                if (fDebugRingSig)
                    LogPrintf("Try again - nTestFee > nFee %d, %d, nTotalBytes %u\n", nTestFee, nFee, nTotalBytes);
                nExpectChangeOuts = vecChange.size();
                return 2; // up changeOutSize
            };

            nFee = nTestFee;

            if (feeMode != 1 && nValue + nFee > nAmountCheck)
            {
                sError = "Not enough (mature) coins with requested ring size to cover amount with fees.";
                if (fDebugRingSig)
                    LogPrintf("Not enough (mature) coins %d with requested ring size to cover amount %d together with fees %d.\n", nAmountCheck, nValue, nFee);
                return 3;
            };


            return 1; // found
        };
    };

    return 0; // not found
};

int CWallet::GetTxnPreImage(CTransaction& txn, uint256& hash)
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << txn.nVersion;
    ss << txn.nTime;
    for (uint32_t i = 0; i < txn.vin.size(); ++i)
    {
        const CTxIn& txin = txn.vin[i];
        ss << txin.prevout; // keyimage only

        int ringSize = txin.ExtractRingSize();

        // TODO: is it neccessary to include the ring members in the hash?
        if (txin.scriptSig.size() < 2 + ringSize * EC_COMPRESSED_SIZE)
        {
            LogPrintf("scriptSig is too small, input %u, ring size %d.\n", i, ringSize);
            return 1;
        };
        ss.write((const char*)&txin.scriptSig[2], ringSize * EC_COMPRESSED_SIZE);
    };

    for (uint32_t i = 0; i < txn.vout.size(); ++i)
        ss << txn.vout[i];
    ss << txn.nLockTime;

    hash = ss.GetHash();

    return 0;
};

bool CWallet::InitMixins(CMixins& mixins, const std::vector<const COwnedAnonOutput*>& vPickedCoins, bool fStaking)
{
    LOCK(cs_main);

    int64_t nStart = GetTimeMicros();
    if (fDebugRingSig)
        LogPrintf("CWallet::InitMixins() : fStaking=%d\n", fStaking);

    CTxDB txdb("r");
    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("CWallet::InitMixins() : cannot get leveldb instance");

    // Init denominations needed and used Txs to skip
    std::set<uint256> setUsedOutputsTxs;
    std::set<int64_t> setDenominations;
    for (const auto & oao : vPickedCoins)
    {
        setUsedOutputsTxs.insert(oao->outpoint.hash);
        setDenominations.insert(oao->nValue);
    }

    leveldb::Iterator *iterator = pdb->NewIterator(txdb.GetReadOptions());

    // Seek to start key.
    CPubKey pkZero;
    pkZero.SetZero();

    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("ao"), pkZero);
    iterator->Seek(ssStartKey.str());

    CPubKey pkAo;
    CAnonOutput anonOutput;
    uint32_t nTotal = 0, nMixins = 0, nInvalid = 0, nUsedTx = 0, nDiffDenom = 0, nImmature = 0, nCompromised = 0;
    while (iterator->Valid())
    {
        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ao")
            break;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        ssKey >> pkAo;
        ssValue >> anonOutput;

        nTotal++;
        if (!pkAo.IsValid())
            nInvalid++;
        else if (setDenominations.find(anonOutput.nValue) == setDenominations.end())
            nDiffDenom++;
        else if (setUsedOutputsTxs.find(anonOutput.outpoint.hash) != setUsedOutputsTxs.end())
            nUsedTx++;
        else
        {
            int nCompromisedHeight = mapAnonOutputStats[anonOutput.nValue].nCompromisedHeight;
            // If hiding outputs are for staking, all outputs must have a enough confirmations for staking
            int minDepth = fStaking || anonOutput.fCoinStake ? Params().GetAnonStakeMinConfirmations() : MIN_ANON_SPEND_DEPTH;
            if (anonOutput.nBlockHeight <= 0 || nBestHeight - anonOutput.nBlockHeight + 1 < minDepth) // ao confirmed in last block has depth of 1
                nImmature++;
            else if (anonOutput.nCompromised != 0 || (nCompromisedHeight != 0 && nCompromisedHeight - MIN_ANON_SPEND_DEPTH >= anonOutput.nBlockHeight))
                nCompromised++;
            else
                try {
                nMixins++;
                mixins.AddAnonOutput(pkAo, anonOutput, nBestHeight);
            } catch (std::exception& e)
            {
                LogPrintf("ERROR: CWallet::InitMixins() : mixins.addAnonOutput threw: %s.\n", e.what());
                return false;
            }
        }
        iterator->Next();
    };

    if (fDebugRingSig)
        LogPrintf("CWallet::InitMixins() : processed %d anons in %d µs; potential mixins: %d; skipped invalid: %d, txUsed: %d, diffDenom: %d, immature: %d, compromised: %d.\n",
                  nTotal, GetTimeMicros() - nStart, nMixins, nInvalid, nUsedTx, nDiffDenom, nImmature, nCompromised);

    delete iterator;
    return true;
}

int CWallet::PickHidingOutputs(CMixins& mixins, int64_t nValue, int nRingSize, int skip, uint8_t* p)
{
    // -- offset skip is pre filled with the real coin
    std::vector<CPubKey> vHideKeys;
    if (!mixins.Pick(nValue, nRingSize - 1, vHideKeys))
        return errorN(1, "%s: Not enough keys found.", __func__);

    for (int i = 0, iMixin = 0; i < nRingSize; ++i)
    {
        if (i == skip)
            continue;

        memcpy(p + i * 33, vHideKeys[iMixin++].begin(), 33);
    };

    return 0;
};

bool CWallet::AreOutputsUnique(CTransaction& txNew)
{
    LOCK(cs_main);
    CTxDB txdb;

    for (uint32_t i = 0; i < txNew.vout.size(); ++i)
    {
        const CTxOut& txout = txNew.vout[i];

        if (!txout.IsAnonOutput())
            continue;

        const CScript &s = txout.scriptPubKey;

        CPubKey pkCoin = CPubKey(&s[2+1], EC_COMPRESSED_SIZE);
        CAnonOutput ao;

        if (txdb.ReadAnonOutput(pkCoin, ao))
        {
            //LogPrintf("AreOutputsUnique() pk %s is not unique.\n", pkCoin);
            return false;
        };
    };

    return true;
};

static int GetRingSigSize(int rsType, int nRingSize)
{
    switch(rsType)
    {
        case RING_SIG_1:
            return 2 + (EC_COMPRESSED_SIZE + EC_SECRET_SIZE + EC_SECRET_SIZE) * nRingSize;
        case RING_SIG_2:
            return 2 + EC_SECRET_SIZE + (EC_COMPRESSED_SIZE + EC_SECRET_SIZE) * nRingSize;
        default:
            LogPrintf("Unknown ring signature type.\n");
            return 0;
    };
};

static uint8_t *GetRingSigPkStart(int rsType, int nRingSize, uint8_t *pStart)
{
    switch(rsType)
    {
        case RING_SIG_1:
            return pStart + 2;
        case RING_SIG_2:
            return pStart + 2 + EC_SECRET_SIZE + EC_SECRET_SIZE * nRingSize;
        default:
            LogPrintf("Unknown ring signature type.\n");
            return 0;
    };
}


bool CWallet::ListAvailableAnonOutputs(std::list<COwnedAnonOutput>& lAvailableAnonOutputs, int64_t& nAmountCheck, int nRingSize, MaturityFilter nFilter, std::string& sError, int64_t nMaxAmount) const
{
    LOCK2(cs_main, cs_wallet);

    nAmountCheck = 0;
    if (ListUnspentAnonOutputs(lAvailableAnonOutputs, nFilter) != 0)
    {
        sError = "ListUnspentAnonOutputs() failed";
        return false;
    };

    // -- remove coins that don't have enough same value anonoutputs in the system for the ring size
    // -- remove coins which spending would lead to ALL SPENT
    int nCoinsPerValue = 0;
    int64_t nLastCoinValue = -1;
    int nMaxSpendable = -1;
    int nAvailableMixins = 0;
    for (std::list<COwnedAnonOutput>::iterator it = lAvailableAnonOutputs.begin(); it != lAvailableAnonOutputs.end();)
    {
        if (nLastCoinValue != it->nValue)
        {
            nCoinsPerValue = 0;
            nLastCoinValue = it->nValue;
            CAnonOutputCount anonOutputCount = mapAnonOutputStats[it->nValue];
            nAvailableMixins = nFilter == MaturityFilter::FOR_STAKING ? anonOutputCount.nMixinsStaking : anonOutputCount.nMixins;
            if (it->nValue <= nMaxAnonOutput)
                nMaxSpendable = (anonOutputCount.nMature - anonOutputCount.nSpends) -
                        (nFilter == MaturityFilter::FOR_STAKING ? 1 : UNSPENT_ANON_SELECT_MIN);
            else
                nMaxSpendable = -1;
            if (fDebugRingSig && nFilter == MaturityFilter::FOR_SPENDING) // called to often when staking
                LogPrintf("ListAvailableAnonOutputs anonValue %d, nAvailableMixins %d, nMaxSpendable %d\n", nLastCoinValue, nAvailableMixins, nMaxSpendable);
        }

        if (nAvailableMixins < nRingSize ||
                (nMaxSpendable != -1 && nCoinsPerValue >= nMaxSpendable) ||
                nAmountCheck + it->nValue > nMaxAmount)
            // -- not enough coins of same value, unspends or over max amount, drop coin
            it = lAvailableAnonOutputs.erase(it);
        else
        {
            nAmountCheck += it->nValue;
            nCoinsPerValue++;
            ++it;
        }
    }

    return true;
}


bool CWallet::AddAnonInput(CMixins& mixins, CTxIn& txin, const COwnedAnonOutput& oao, int rsType, int nRingSize, int& oaoRingIndex, bool fStaking, bool fTestOnly, std::string& sError)
{
    int nSigSize = GetRingSigSize(rsType, nRingSize);

    // -- overload prevout to hold keyImage
    memcpy(txin.prevout.hash.begin(), &oao.vchImage[0], EC_SECRET_SIZE);

    txin.prevout.n = 0 | ((oao.vchImage[32]) & 0xFF) | (int32_t)(((int16_t) nRingSize) << 16);

    // -- size for full signature, signature is added later after hash
    try { txin.scriptSig.resize(nSigSize); } catch (std::exception& e)
    {
        LogPrintf("Error: AddAnonInput() txin.scriptSig.resize threw: %s.\n", e.what());
        sError = "resize failed.\n";
        return false;
    };

    txin.scriptSig[0] = OP_RETURN;
    txin.scriptSig[1] = OP_ANON_MARKER;

    if (fTestOnly)
        return true;

    int nCoinOutId = oao.outpoint.n;
    WalletTxMap::iterator mi = mapWallet.find(oao.outpoint.hash);
    if (mi == mapWallet.end()
        || mi->second.nVersion != ANON_TXN_VERSION
        || (int)mi->second.vout.size() < nCoinOutId)
    {
        LogPrintf("Error: AddAnonInput() picked coin not in wallet, %s version %d.\n", oao.outpoint.hash.ToString().c_str(), (*mi).second.nVersion);
        sError = "picked coin not in wallet.\n";
        return false;
    };

    CWalletTx& wtxAnonCoin = mi->second;

    const CTxOut& txout = wtxAnonCoin.vout[nCoinOutId];
    const CScript &s = txout.scriptPubKey;

    if (!txout.IsAnonOutput())
    {
        sError = "picked coin not an anon output.\n";
        return false;
    };

    CPubKey pkCoin = CPubKey(&s[2+1], EC_COMPRESSED_SIZE);

    if (!pkCoin.IsValid())
    {
        sError = "pkCoin is invalid.\n";
        return false;
    };

    oaoRingIndex = GetRand(nRingSize);

    uint8_t *pPubkeyStart = GetRingSigPkStart(rsType, nRingSize, &txin.scriptSig[0]);

    memcpy(pPubkeyStart + oaoRingIndex * EC_COMPRESSED_SIZE, pkCoin.begin(), EC_COMPRESSED_SIZE);

    if (PickHidingOutputs(mixins, oao.nValue, nRingSize, oaoRingIndex, pPubkeyStart) != 0)
    {
        sError = "PickHidingOutputs() failed.\n";
        return false;
    }

    return true;
}

bool CWallet::GenerateRingSignature(CTxIn& txin, const int& rsType, const int& nRingSize, const int& nSecretOffset, const uint256& preimage, std::string& sError)
{
    // Test
    std::vector<uint8_t> vchImageTest;
    txin.ExtractKeyImage(vchImageTest);

    int nTestRingSize = txin.ExtractRingSize();
    if (nTestRingSize != nRingSize)
    {
        sError = "nRingSize embed error.";
        return false;
    };

    int nSigSize = GetRingSigSize(rsType, nRingSize);
    if (txin.scriptSig.size() < (unsigned long) nSigSize)
    {
        sError = "Error: scriptSig too small.";
        return false;
    };

    uint8_t *pPubkeyStart = GetRingSigPkStart(rsType, nRingSize, &txin.scriptSig[0]);

    // -- get secret
    CPubKey pkCoin = CPubKey(pPubkeyStart + EC_COMPRESSED_SIZE * nSecretOffset, EC_COMPRESSED_SIZE);
    CKeyID pkId = pkCoin.GetID();

    CKey key;
    if (!GetKey(pkId, key))
    {
        sError = "Error: don't have key for output.";
        return false;
    };

    ec_secret ecSecret;
    if (key.size() != EC_SECRET_SIZE)
    {
        sError = "Error: key.size() != EC_SECRET_SIZE.";
        return false;
    };

    memcpy(&ecSecret.e[0], key.begin(), key.size());

    switch(rsType)
    {
        case RING_SIG_1:
            {
            uint8_t *pPubkeys = &txin.scriptSig[2];
            uint8_t *pSigc    = &txin.scriptSig[2 + EC_COMPRESSED_SIZE * nRingSize];
            uint8_t *pSigr    = &txin.scriptSig[2 + (EC_COMPRESSED_SIZE + EC_SECRET_SIZE) * nRingSize];
            if (generateRingSignature(vchImageTest, preimage, nRingSize, nSecretOffset, ecSecret, pPubkeys, pSigc, pSigr) != 0)
            {
                sError = "Error: generateRingSignature() failed.";
                return false;
            };
            // -- test verify
            if (verifyRingSignature(vchImageTest, preimage, nRingSize, pPubkeys, pSigc, pSigr) != 0)
            {
                sError = "Error: verifyRingSignature() failed.";
                return false;
            };
            }
            break;
        case RING_SIG_2:
            {
            ec_point pSigC;
            uint8_t *pSigS    = &txin.scriptSig[2 + EC_SECRET_SIZE];
            uint8_t *pPubkeys = &txin.scriptSig[2 + EC_SECRET_SIZE + EC_SECRET_SIZE * nRingSize];
            if (generateRingSignatureAB(vchImageTest, nRingSize, nSecretOffset, ecSecret, pPubkeys, pSigC, pSigS) != 0)
            {
                sError = "Error: generateRingSignatureAB() failed.";
                return false;
            };
            if (pSigC.size() == EC_SECRET_SIZE)
                memcpy(&txin.scriptSig[2], &pSigC[0], EC_SECRET_SIZE);
            else
                LogPrintf("pSigC.size() : %d Invalid!!\n", pSigC.size());

            // -- test verify
            if (verifyRingSignatureAB(vchImageTest, nRingSize, pPubkeys, pSigC, pSigS) != 0)
            {
                sError = "Error: verifyRingSignatureAB() failed.";
                return false;
            };
            }
            break;
        default:
            sError = "Unknown ring signature type.";
            return false;
    };

    memset(&ecSecret.e[0], 0, EC_SECRET_SIZE); // optimised away?

    return true;
}

bool CWallet::AddAnonInputs(int rsType, int64_t nTotalOut, int nRingSize, const std::vector<std::pair<CScript, int64_t> >&vecSend, std::vector<std::pair<CScript, int64_t> >&vecChange, CWalletTx& wtxNew, int64_t& nFeeRequired, bool fTestOnly, std::string& sError)
{
    int64_t nStart = GetTimeMicros();
    if (fDebugRingSig)
        LogPrintf("AddAnonInputs() %d, %d, rsType:%d\n", nTotalOut, nRingSize, rsType);

    auto [nMinRingSize, nMaxRingSize] = GetRingSizeMinMax();
    if (nRingSize < (int)nMinRingSize || nRingSize > (int)nMaxRingSize)
    {
        sError = tfm::format("Ringsize %d not in range [%d, %d]: ", nRingSize, nMinRingSize, nMaxRingSize);
        return false;
    }

    std::list<COwnedAnonOutput> lAvailableCoins;
    int64_t nAmountCheck;
    if (!ListAvailableAnonOutputs(lAvailableCoins, nAmountCheck, nRingSize, MaturityFilter::FOR_SPENDING, sError))
        return false;
    if (fDebugRingSig)
        LogPrintf("Debug: CWallet::AddAnonInputs() : ListAvailableAnonOutputs() : %d anons picked in %d µs, total %d.\n", lAvailableCoins.size(), GetTimeMicros() - nStart, nAmountCheck);

    // -- estimate fee

    uint32_t nSizeOutputs = 0;
    for (uint32_t i = 0; i < vecSend.size(); ++i) // need to sum due to narration
        nSizeOutputs += GetSizeOfCompactSize(vecSend[i].first.size()) + vecSend[i].first.size() + sizeof(int64_t); // CTxOut

    int64_t nStartPickAnon = GetTimeMicros();
    bool fFound = false;
    int64_t nFee;
    int nExpectChangeOuts = 1;
    std::string sPickError;
    std::vector<const COwnedAnonOutput*> vPickedCoins;
    for (int k = 0; k < 50; ++k) // safety
    {
        // -- nExpectChangeOuts is raised if needed (rv == 2)
        int rv = PickAnonInputs(rsType, nTotalOut, nFee, nRingSize, wtxNew, vecSend.size(), nSizeOutputs, nExpectChangeOuts, lAvailableCoins, vPickedCoins, vecChange, false, sPickError);
        if (rv == 0)
            break;
        if (rv == 3)
        {
            nFeeRequired = nFee; // set in PickAnonInputs()
            sError = sPickError;
            return false;
        };
        if (rv == 1)
        {
            fFound = true;
            break;
        };
    };
    if (fDebugRingSig)
        LogPrintf("Debug: CWallet::AddAnonInputs() : PickAnonInputs() : picked %d anons in %d µs.\n", vPickedCoins.size(), GetTimeMicros() - nStartPickAnon);

    if (!fFound)
    {
        sError = "No combination of (mature) coins matches amount and ring size.";
        return false;
    };

    nFeeRequired = nFee; // set in PickAnonInputs()

    // -- need hash of tx without signatures
    std::vector<int> vCoinOffsets;
    uint32_t ii = 0;
    wtxNew.vin.resize(vPickedCoins.size());
    vCoinOffsets.resize(vPickedCoins.size());

    // -- Initialize mixins set
    CMixins mixins;
    if (!InitMixins(mixins, vPickedCoins, false))
        return false;

    int64_t nStartPickMixins = GetTimeMicros();
    for (std::vector<const COwnedAnonOutput*>::iterator it = vPickedCoins.begin(); it != vPickedCoins.end(); ++it)
    {
        if (fDebugRingSig)
            LogPrintf("pickedCoin %s %d\n", HexStr((*it)->vchImage).c_str(), (*it)->nValue);

        if (!AddAnonInput(mixins, wtxNew.vin[ii], *(*it), rsType, nRingSize, vCoinOffsets[ii], false, fTestOnly, sError))
            return false;

        ii++;
    };
    if (fDebugRingSig)
        LogPrintf("Debug: CWallet::AddAnonInputs() : AddAnonInput(): picked mixins for %d ring signatures with ring size %d in %d µs.\n", vPickedCoins.size(), nRingSize, GetTimeMicros() - nStartPickMixins);

    for (uint32_t i = 0; i < vecSend.size(); ++i)
        wtxNew.vout.push_back(CTxOut(vecSend[i].second, vecSend[i].first));
    for (uint32_t i = 0; i < vecChange.size(); ++i)
        wtxNew.vout.push_back(CTxOut(vecChange[i].second, vecChange[i].first));

    std::sort(wtxNew.vout.begin(), wtxNew.vout.end());

    if (fTestOnly)
        return true;

    uint256 preimage;
    if (GetTxnPreImage(wtxNew, preimage) != 0)
    {
        sError = "GetPreImage() failed.\n";
        return false;
    };

    // TODO: Does it lower security to use the same preimage for each input?
    //  cryptonote seems to do so too
    int64_t nStartRingSig = GetTimeMicros();
    for (uint32_t i = 0; i < wtxNew.vin.size(); ++i)
    {
        if (!GenerateRingSignature(wtxNew.vin[i], rsType, nRingSize, vCoinOffsets[i], preimage, sError))
            return false;
    };
    if (fDebugRingSig)
        LogPrintf("Debug: CWallet::AddAnonInputs() : GenerateRingSignature() : generated %d ring signatures with ring size %d in %d µs.\n", wtxNew.vin.size(), nRingSize, GetTimeMicros() - nStartRingSig);

    // -- check if new coins already exist (in case random is broken ?)
    if (!AreOutputsUnique(wtxNew))
    {
        sError = "Error: Anon outputs are not unique - is random working!.";
        return false;
    };

    if (fDebugRingSig)
        LogPrintf("Debug: CWallet::AddAnonInputs() : finished in %d µs.\n", GetTimeMicros() - nStart);
    return true;
};

bool CWallet::SendSpecToAnon(CStealthAddress& sxAddress, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    if (fDebugRingSig)
        LogPrintf("SendSpecToAnon()\n");

    if (IsLocked())
    {
        sError = _("Error: Wallet locked, unable to create transaction.");
        return false;
    };

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    if (fWalletUnlockStakingOnly)
    {
        sError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        return false;
    };

    if (nBestHeight < GetNumBlocksOfPeers()-1)
    {
        sError = _("Error: Block chain must be fully synced first.");
        return false;
    };

    if (vNodes.empty())
    {
        sError = _("Error: Alias is not connected!");
        return false;
    };


    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetBalance())
    {
        sError = "Insufficient funds";
        return false;
    };

    wtxNew.nVersion = ANON_TXN_VERSION;

    CScript scriptNarration; // needed to match output id of narr
    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::map<CKeyID, CStealthAddress> mapPubStealth;

    if (!CreateAnonOutputs(&sxAddress, nValue, sNarr, vecSend, scriptNarration, &mapPubStealth))
    {
        sError = "CreateAnonOutputs() failed.";
        return false;
    };


    // -- shuffle outputs
// Removed with c++17, see https://en.cppreference.com/w/cpp/algorithm/random_shuffle
//    std::random_shuffle(vecSend.begin(), vecSend.end());
    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(vecSend.begin(), vecSend.end(), urng);

    int64_t nFeeRequired;
    int nChangePos;
    if (!CreateTransaction(vecSend, wtxNew, nFeeRequired, nChangePos, NULL))
    {
        sError = "CreateTransaction() failed.";
        return false;
    };

    if (!SaveNarrationOutput(wtxNew, scriptNarration, sNarr, sError))
        return false;

    if (fAskFee && !uiInterface.ThreadSafeAskFee(nFeeRequired, _("Sending...")))
    {
        sError = "ABORTED";
        return false;
    };

    // -- check if new coins already exist (in case random is broken ?)
    if (!AreOutputsUnique(wtxNew))
    {
        sError = "Error: Anon outputs are not unique - is random working!.";
        return false;
    };

    if (!CommitTransaction(wtxNew, &mapPubStealth))
    {
        sError = "Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        UndoAnonTransaction(wtxNew, &mapPubStealth);
        return false;
    };

    return true;
};

bool CWallet::SendAnonToAnon(CStealthAddress& sxAddress, int64_t nValue, int nRingSize, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    if (fDebugRingSig)
        LogPrintf("SendAnonToAnon()\n");

    if (IsLocked())
    {
        sError = _("Error: Wallet locked, unable to create transaction.");
        return false;
    };

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    if (fWalletUnlockStakingOnly)
    {
        sError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        return false;
    };

    if (nBestHeight < GetNumBlocksOfPeers()-1)
    {
        sError = _("Error: Block chain must be fully synced first.");
        return false;
    };

    if (vNodes.empty())
    {
        sError = _("Error: Alias is not connected!");
        return false;
    };

    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetSpectreBalance())
    {
        sError = "Insufficient SPECTRE funds";
        return false;
    };

    wtxNew.nVersion = ANON_TXN_VERSION;

    CScript scriptNarration; // needed to match output id of narr
    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::vector<std::pair<CScript, int64_t> > vecChange;
    std::map<CKeyID, CStealthAddress> mapPubStealth;

    if (!CreateAnonOutputs(&sxAddress, nValue, sNarr, vecSend, scriptNarration, &mapPubStealth))
    {
        sError = "CreateAnonOutputs() failed.";
        return false;
    };


    // -- shuffle outputs (any point?)
    //std::random_shuffle(vecSend.begin(), vecSend.end());

    int64_t nFeeRequired;
    std::string sError2;
    if (!AddAnonInputs(nRingSize == 1 ? RING_SIG_1 : RING_SIG_2, nValue, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, false, sError2))
    {
        LogPrintf("SendAnonToAnon() AddAnonInputs failed %s.\n", sError2.c_str());
        sError = "AddAnonInputs() failed : " + sError2;
        return false;
    };

    if (!SaveNarrationOutput(wtxNew, scriptNarration, sNarr, sError2))
    {
        LogPrintf("SendAnonToAnon() SaveNarrationOutput failed %s.\n", sError.c_str());
        sError = "SaveNarrationOutput() failed : " + sError2;
        return false;
    }
    if (!CommitTransaction(wtxNew, &mapPubStealth))
    {
        sError = "Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        UndoAnonTransaction(wtxNew, &mapPubStealth);
        return false;
    };

    return true;
};

bool CWallet::SendAnonToSpec(CStealthAddress& sxAddress, int64_t nValue, int nRingSize, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    if (fDebug)
        LogPrintf("SendAnonToSpec()\n");

    if (IsLocked())
    {
        sError = _("Error: Wallet locked, unable to create transaction.");
        return false;
    };

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    if (fWalletUnlockStakingOnly)
    {
        sError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        return false;
    };

    if (nBestHeight < GetNumBlocksOfPeers()-1)
    {
        sError = _("Error: Block chain must be fully synced first.");
        return false;
    };

    if (vNodes.empty())
    {
        sError = _("Error: Alias is not connected!");
        return false;
    };

    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetSpectreBalance())
    {
        sError = "Insufficient SPECTRE funds";
        return false;
    };

    std::ostringstream ssThrow;
    auto [nMinRingSize, nMaxRingSize] = GetRingSizeMinMax();
    if (nRingSize < (int)nMinRingSize || nRingSize > (int)nMaxRingSize)
    {
        sError = nMinRingSize == nMaxRingSize ? tfm::format("Ring size must be = %d.", nMinRingSize) :
                                                tfm::format("Ring size must be >= %d and <= %d.", nMinRingSize, nMaxRingSize);
        return false;
    }

    wtxNew.nVersion = ANON_TXN_VERSION;

    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::vector<std::pair<CScript, int64_t> > vecChange;
    std::map<CScript, std::string> mapScriptNarr;
    CScript scriptNarration;
    std::string sError2;
    if (!CreateStealthOutput(&sxAddress, nValue, sNarr, vecSend, scriptNarration, sError2))
    {
        LogPrintf("SendAnonToSpec() CreateStealthOutput failed %s.\n", sError2.c_str());
        sError = "CreateStealthOutput() failed : " + sError2;
        return false;
    };

    if (!SaveNarrationOutput(wtxNew, scriptNarration, sNarr, sError2))
    {
        LogPrintf("SendAnonToSpec() SaveNarrationOutput failed %s.\n", sError2.c_str());
        sError = "SaveNarrationOutput() failed : " + sError2;
        return false;
    }

    // -- get anon inputs

    int64_t nFeeRequired;
    if (!AddAnonInputs(nRingSize == 1 ? RING_SIG_1 : RING_SIG_2, nValue, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, false, sError2))
    {
        LogPrintf("SendAnonToSpec() AddAnonInputs failed %s.\n", sError2.c_str());
        sError = "AddAnonInputs() failed: " + sError2;
        return false;
    };

    if (!CommitTransaction(wtxNew))
    {
        sError = "Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        UndoAnonTransaction(wtxNew);
        return false;
    };

    return true;
};

bool CWallet::SaveNarrationOutput(CWalletTx& wtxNew, const CScript& scriptNarration, const std::string& sNarr, std::string& sError)
{
    if (scriptNarration.size() > 0)
    {
        for (uint32_t k = 0; k < wtxNew.vout.size(); ++k)
        {
            if (wtxNew.vout[k].scriptPubKey != scriptNarration)
                continue;
            char key[64];
            if (snprintf(key, sizeof(key), "n_%u", k) < 1)
            {
                sError = "Error creating narration key.";
                return false;
            };
            wtxNew.mapValue[key] = sNarr;
            break;
        }
    }
    return true;
}

bool CWallet::ExpandLockedAnonOutput(CWalletDB *pwdb, CKeyID &ckeyId, CLockedAnonOutput &lao, std::set<uint256> &setUpdated)
{
    if (fDebugRingSig)
    {
        CBitcoinAddress addrTo(ckeyId);
        LogPrintf("%s %s\n", __func__, addrTo.ToString().c_str());
        AssertLockHeld(cs_main);
        AssertLockHeld(cs_wallet);
    };

    CStealthAddress sxFind;
    sxFind.SetScanPubKey(lao.pkScan);

    bool fFound = false;
    ec_secret sSpendR;
    ec_secret sSpend;
    ec_secret sScan;

    ec_point pkEphem;


    std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxFind);
    if (si != stealthAddresses.end())
    {
        fFound = true;

        if (si->spend_secret.size() != EC_SECRET_SIZE
         || si->scan_secret .size() != EC_SECRET_SIZE)
            return error("%s: Stealth address has no secret.", __func__);

        memcpy(&sScan.e[0], &si->scan_secret[0], EC_SECRET_SIZE);
        memcpy(&sSpend.e[0], &si->spend_secret[0], EC_SECRET_SIZE);

        pkEphem.resize(lao.pkEphem.size());
        memcpy(&pkEphem[0], lao.pkEphem.begin(), lao.pkEphem.size());

        if (StealthSecretSpend(sScan, pkEphem, sSpend, sSpendR) != 0)
            return error("%s: StealthSecretSpend() failed.", __func__);

    };

    // - check ext account stealth keys
    ExtKeyAccountMap::const_iterator mi;
    if (!fFound)
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        fFound = true;

        CExtKeyAccount *ea = mi->second;

        CKeyID sxId = lao.pkScan.GetID();

        AccStealthKeyMap::const_iterator miSk = ea->mapStealthKeys.find(sxId);
        if (miSk == ea->mapStealthKeys.end())
            continue;

        const CEKAStealthKey &aks = miSk->second;
        if (ea->IsLocked(aks))
            return error("%s: Stealth is locked.", __func__);

        ec_point pkExtracted;
        ec_secret sShared;

        pkEphem.resize(lao.pkEphem.size());
        memcpy(&pkEphem[0], lao.pkEphem.begin(), lao.pkEphem.size());
        memcpy(&sScan.e[0], aks.skScan.begin(), EC_SECRET_SIZE);

        // - need sShared to extract key
        if (StealthSecret(sScan, pkEphem, aks.pkSpend, sShared, pkExtracted) != 0)
            return error("%s: StealthSecret() failed.", __func__);

        CKey kChild;

        if (0 != ea->ExpandStealthChildKey(&aks, sShared, kChild))
            return error("%s: ExpandStealthChildKey() failed %s.", __func__, aks.ToStealthAddress().c_str());

        memcpy(&sSpendR.e[0], kChild.begin(), EC_SECRET_SIZE);
    };


    if (!fFound)
        return error("%s: No stealth key found.", __func__);

    ec_point pkTestSpendR;
    if (SecretToPublicKey(sSpendR, pkTestSpendR) != 0)
        return error("%s: SecretToPublicKey() failed.", __func__);

    CKey ckey;
    ckey.Set(&sSpendR.e[0], true);
    if (!ckey.IsValid())
        return error("%s: Reconstructed key is invalid.", __func__);

    CPubKey pkCoin = ckey.GetPubKey(true);
    if (!pkCoin.IsValid())
        return error("%s: pkCoin is invalid.", __func__);

    CKeyID keyIDTest = pkCoin.GetID();
    if (keyIDTest != ckeyId)
    {
        LogPrintf("%s: Error: Generated secret does not match.\n", __func__);
        if (fDebugRingSig)
        {
            LogPrintf("test   %s\n", keyIDTest.ToString().c_str());
            LogPrintf("gen    %s\n", ckeyId.ToString().c_str());
        };
        return false;
    };

    if (fDebugRingSig)
    {
        CBitcoinAddress coinAddress(keyIDTest);
        LogPrintf("Adding secret to key %s.\n", coinAddress.ToString().c_str());
    };

    if (!AddKeyInDBTxn(pwdb, ckey))
        return error("%s: AddKeyInDBTxn failed.", __func__);

    // -- store keyimage
    ec_point pkImage;
    ec_point pkOldImage;
    getOldKeyImage(pkCoin, pkOldImage);
    if (generateKeyImage(pkTestSpendR, sSpendR, pkImage) != 0)
        return error("%s: generateKeyImage failed.", __func__);

    bool fSpentAOut = false;


    setUpdated.insert(lao.outpoint.hash);

    {
        // -- check if this output is already spent
        CTxDB txdb;

        CKeyImageSpent kis;

        bool fInMemPool;
        CAnonOutput ao;
        txdb.ReadAnonOutput(pkCoin, ao);
        if ((GetKeyImage(&txdb, pkImage, kis, fInMemPool) && !fInMemPool)
          ||(GetKeyImage(&txdb, pkOldImage, kis, fInMemPool) && !fInMemPool)) // shouldn't be possible for kis to be in mempool here
        {
            fSpentAOut = true;

            WalletTxMap::iterator miw = mapWallet.find(lao.outpoint.hash);
            if (miw != mapWallet.end())
            {
                CWalletTx& wtx = (*miw).second;
                wtx.MarkSpent(lao.outpoint.n);

                if (!pwdb->WriteTx(lao.outpoint.hash, wtx))
                    return error("%s: WriteTx %s failed.", __func__, wtx.ToString().c_str());

                wtx.MarkDirty();
            };
        };
    } // txdb

    COwnedAnonOutput oao(lao.outpoint, fSpentAOut);
    if (!pwdb->WriteOwnedAnonOutput(pkImage, oao)
      ||!pwdb->WriteOldOutputLink(pkOldImage, pkImage)
      ||!pwdb->WriteOwnedAnonOutputLink(pkCoin, pkImage))
    {
        return error("%s: WriteOwnedAnonOutput() failed.", __func__);
    };

    if (fDebugRingSig)
        LogPrintf("Adding anon output to wallet: %s.\n", HexStr(pkImage).c_str());

    return true;
};

bool CWallet::ProcessLockedAnonOutputs()
{
    if (fDebugRingSig)
    {
        LogPrintf("%s\n", __func__);
        AssertLockHeld(cs_main);
        AssertLockHeld(cs_wallet);
    };
    // -- process owned anon outputs received when wallet was locked.


    std::set<uint256> setUpdated;

    CWalletDB walletdb(strWalletFile, "cr+");
    walletdb.TxnBegin();
    Dbc *pcursor = walletdb.GetTxnCursor();

    if (!pcursor)
        throw runtime_error(strprintf("%s : cannot create DB cursor.", __func__).c_str());
    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("lao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error(strprintf("%s : error scanning DB.", __func__).c_str());
        };

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "lao")
            break;
        CLockedAnonOutput lockedAnonOutput;
        CKeyID ckeyId;
        ssKey >> ckeyId;
        ssValue >> lockedAnonOutput;

        if (ExpandLockedAnonOutput(&walletdb, ckeyId, lockedAnonOutput, setUpdated))
        {
            if ((ret = pcursor->del(0)) != 0)
               LogPrintf("%s : Delete failed %d, %s\n", __func__, ret, db_strerror(ret));
        };
    };

    pcursor->close();

    walletdb.TxnCommit();

    std::set<uint256>::iterator it;
    for (it = setUpdated.begin(); it != setUpdated.end(); ++it)
    {
        WalletTxMap::iterator miw = mapWallet.find(*it);
        if (miw == mapWallet.end())
            continue;
        CWalletTx& wtx = (*miw).second;
        wtx.MarkDirty();
        wtx.fDebitCached = 2; // force update

        NotifyTransactionChanged(this, *it, CT_UPDATED);
    };

    return true;
};

bool CWallet::EstimateAnonFee(int64_t nValue, int nRingSize, std::string& sNarr, CWalletTx& wtxNew, int64_t& nFeeRet, std::string& sError)
{
    if (fDebugRingSig)
        LogPrintf("EstimateAnonFee()\n");

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    nFeeRet = 0;

    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetSpectreBalance())
    {
        sError = "Insufficient SPECTRE funds";
        return false;
    };

    CScript scriptNarration; // needed to match output id of narr
    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::vector<std::pair<CScript, int64_t> > vecChange;

    if (!CreateAnonOutputs(NULL, nValue, sNarr, vecSend, scriptNarration))
    {
        sError = "CreateAnonOutputs() failed.";
        return false;
    };

    int64_t nFeeRequired;
    if (!AddAnonInputs(nRingSize == 1 ? RING_SIG_1 : RING_SIG_2, nValue, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, true, sError))
    {
        LogPrintf("EstimateAnonFee() AddAnonInputs failed %s.\n", sError.c_str());
        sError = "AddAnonInputs() failed.";
        return false;
    };

    nFeeRet = nFeeRequired;

    return true;
};

int CWallet::ListUnspentAnonOutputs(std::list<COwnedAnonOutput>& lUAnonOutputs, MaturityFilter nFilter) const
{
    LOCK(cs_wallet);

    CWalletDB walletdb(strWalletFile, "r");

    Dbc* pcursor = walletdb.GetAtCursor();
    if (!pcursor)
        throw runtime_error("CWallet::ListUnspentAnonOutputs() : cannot create DB cursor");
    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("oao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("CWallet::ListUnspentAnonOutputs() : error scanning DB");
        };

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "oao")
            break;
        COwnedAnonOutput oao;
        ssKey >> oao.vchImage;

        ssValue >> oao;

        if (oao.fSpent)
            continue;

        WalletTxMap::const_iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi == mapWallet.end()
            || mi->second.nVersion != ANON_TXN_VERSION
            || mi->second.vout.size() <= oao.outpoint.n
            || mi->second.IsSpent(oao.outpoint.n))
            continue;


        // -- Check maturity
        if (nFilter != NONE)
        {
            // maturity (minDepth) depends on if the output was created in a staking transaction or is used for staking
            int minBlockHeight = mi->second.IsCoinStake() || nFilter == MaturityFilter::FOR_STAKING ?
                        Params().GetAnonStakeMinConfirmations() : MIN_ANON_SPEND_DEPTH;

            if (mi->second.GetDepthInMainChain() < minBlockHeight)
                continue;
        }

        // TODO: check ReadAnonOutput?

        oao.nValue = mi->second.vout[oao.outpoint.n].nValue;


        // -- insert by nValue asc
        bool fInserted = false;
        for (std::list<COwnedAnonOutput>::iterator it = lUAnonOutputs.begin(); it != lUAnonOutputs.end(); ++it)
        {
            if (oao.nValue > it->nValue)
                continue;
            lUAnonOutputs.insert(it, oao);
            fInserted = true;
            break;
        };
        if (!fInserted)
            lUAnonOutputs.push_back(oao);
    };

    pcursor->close();
    return 0;
}

int CWallet::CountAnonOutputs(std::map<int64_t, int>& mOutputCounts, MaturityFilter nFilter) const
{
    LOCK(cs_main);
    CTxDB txdb("r");

    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("CWallet::CountAnonOutputs() : cannot get leveldb instance");

    leveldb::Iterator *iterator = pdb->NewIterator(txdb.GetReadOptions());


    // Seek to start key.
    CPubKey pkZero;
    pkZero.SetZero();

    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("ao"), pkZero);
    iterator->Seek(ssStartKey.str());


    while (iterator->Valid())
    {
        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ao")
            break;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        CAnonOutput anonOutput;
        ssValue >> anonOutput;

        // maturity (minDepth) depends on if the output was created in a staking transaction or is used for staking
        int minBlockHeight = anonOutput.fCoinStake || nFilter == MaturityFilter::FOR_STAKING ?
                    Params().GetAnonStakeMinConfirmations() : MIN_ANON_SPEND_DEPTH;

        // Don't count anons which are compromised by ALL SPENT
        int nCompromisedHeight = mapAnonOutputStats[anonOutput.nValue].nCompromisedHeight;

        if ((nFilter == MaturityFilter::NONE ||
             (anonOutput.nBlockHeight > 0 && nBestHeight - anonOutput.nBlockHeight + 1 >= minBlockHeight)) // ao confirmed in last block has depth of 1
                && (Params().IsProtocolV3(nBestHeight) ? anonOutput.nCompromised == 0 : true)
                && (nCompromisedHeight == 0 || anonOutput.nBlockHeight > nCompromisedHeight - MIN_ANON_SPEND_DEPTH))
        {
            std::map<int64_t, int>::iterator mi = mOutputCounts.find(anonOutput.nValue);
            if (mi != mOutputCounts.end())
                mi->second++;
        };

        iterator->Next();
    };

    delete iterator;

    return 0;
};

int CWallet::CountAllAnonOutputs(std::list<CAnonOutputCount>& lOutputCounts, int nBlockHeight, std::function<void (const unsigned mode, const uint32_t&)> funcProgress)
{
    auto start = std::chrono::high_resolution_clock::now();
    int64_t nTotalAoRead = 0;
    int64_t nTotalKiRead = 0;

    // TODO: there are few enough possible coin values to preinitialise a vector with all of them

    LOCK(cs_main);
    CTxDB txdb("r");

    // initialize mapAnonBlockStats for last nMaxAnonBlockCache blocks
    mapAnonBlockStats.clear();
    for (int i = nBlockHeight;i >= 0 && i >= nBlockHeight - nMaxAnonBlockCache;--i)
        mapAnonBlockStats[i];

    std::map<int64_t, std::vector<int>> mapCompromisedHeights;
    txdb.ReadCompromisedAnonHeights(mapCompromisedHeights);

    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("CWallet::CountAnonOutputs() : cannot get leveldb instance");

    leveldb::Iterator *iterator = pdb->NewIterator(txdb.GetReadOptions());

    // Seek to start key.
    CPubKey pkZero;
    pkZero.SetZero();

    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("ao"), pkZero);
    iterator->Seek(ssStartKey.str());

    uint32_t count = 0;
    while (iterator->Valid())
    {   
         if (funcProgress && count != 0 && count % 10000 == 0) funcProgress(0, count);
         count++;

        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ao")
            break;

        nTotalAoRead++;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        CAnonOutput ao;
        ssValue >> ao;

        int nCompromisedHeight = 0;
        if (ao.nBlockHeight)
        {
            // ao confirmed in blocks after the given nBlockHeight are skipped
            if (ao.nBlockHeight > nBlockHeight)
            {
                iterator->Next();
                continue;
            }

            // Check if ao is compromised as mixin by ALL SPENT
            std::map<int64_t, std::vector<int>>::iterator it = mapCompromisedHeights.find(ao.nValue);
            if (it != mapCompromisedHeights.end() && it->second.size() > 0)
                nCompromisedHeight = it->second.front();
        }
        bool fCompromised = ao.nCompromised != 0 || (nCompromisedHeight != 0 && nCompromisedHeight - MIN_ANON_SPEND_DEPTH >= ao.nBlockHeight);
        int nDepth = ao.nBlockHeight > 0 ? nBlockHeight - ao.nBlockHeight + 1: 0; // ao confirmed in last block has depth of 1
        // maturity for spending (minDepth) depends on if the output was created in a staking transaction
        int nMature = nDepth >= (ao.fCoinStake ? Params().GetAnonStakeMinConfirmations() : MIN_ANON_SPEND_DEPTH);
        // mixins defines if this ao is usable as mixin for spending.
        int nMixins = nMature && !fCompromised;
         // mixinsStaking defines if this ao is usable as mixin for staking.
        int nMixinsStaking = nMixins && nDepth >= Params().GetAnonStakeMinConfirmations();

        int nExists = 0, nUnconfirmed = 0;
        if  (ao.nBlockHeight)
            nExists++;
        else
            nUnconfirmed++;

        // -- insert by nValue asc
        bool fProcessed = false;
        for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
        {
            if (ao.nValue == it->nValue)
            {
                it->nExists += nExists;
                it->nUnconfirmed += nUnconfirmed;
                it->nCompromised += fCompromised;
                it->nMature += nMature;
                it->nMixins += nMixins;
                it->nMixinsStaking += nMixinsStaking;
                it->nStakes += ao.fCoinStake;
                if (it->nLastHeight < ao.nBlockHeight)
                    it->nLastHeight = ao.nBlockHeight;
                fProcessed = true;
                break;
            };
            if (ao.nValue > it->nValue)
                continue;
            lOutputCounts.insert(it, CAnonOutputCount(ao.nValue, nExists, nUnconfirmed, 0, 0, ao.nBlockHeight, fCompromised, nMature, nMixins, nMixinsStaking, ao.fCoinStake, 0));
            fProcessed = true;
            break;
        };
        if (!fProcessed)
            lOutputCounts.push_back(CAnonOutputCount(ao.nValue, nExists, nUnconfirmed, 0, 0, ao.nBlockHeight, fCompromised, nMature, nMixins, nMixinsStaking, ao.fCoinStake, 0));


        // add last 1000 anon blocks to mapAnonBlockStats
        if (ao.nBlockHeight && nBlockHeight - ao.nBlockHeight <= nMaxAnonBlockCache)
        {
            CAnonBlockStat& anonBlockStat = mapAnonBlockStats[ao.nBlockHeight][ao.nValue];
            anonBlockStat.nCompromisedOutputs += fCompromised;
            if (ao.fCoinStake)
                anonBlockStat.nStakingOutputs++;
            else
                anonBlockStat.nOutputs++;
        }

        iterator->Next();
    };
    if (funcProgress) funcProgress(0, count);

    delete iterator;


    // -- count spends

    iterator = pdb->NewIterator(txdb.GetReadOptions());
    ssStartKey.clear();
    ssStartKey << make_pair(string("ki"), pkZero);
    iterator->Seek(ssStartKey.str());

    count = 0;
    while (iterator->Valid())
    {   
        if (funcProgress && count != 0 && count % 10000 == 0) funcProgress(1, count);
        count++;

        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ki")
            break;

        nTotalKiRead++;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        CKeyImageSpent kis;
        ssValue >> kis;

        // ki in blocks after the given nBlockHeight are skipped
        if (kis.nBlockHeight <= nBlockHeight)
        {
            bool fProcessed = false;
            for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
            {
                if (kis.nValue != it->nValue)
                    continue;
                it->nSpends++;
                fProcessed = true;
                break;
            };
            if (!fProcessed)
                LogPrintf("WARNING: CountAllAnonOutputs found keyimage without matching anon output value.\n");
        }

        iterator->Next();
    };
    if (funcProgress) funcProgress(1, count);

    delete iterator;

    // set compromised anon block heights
    for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
    {
        std::map<int64_t, std::vector<int>>::iterator itCompromisedHeights = mapCompromisedHeights.find(it->nValue);
        if (itCompromisedHeights != mapCompromisedHeights.end() && itCompromisedHeights->second.size() > 0)
        {
           it->nCompromisedHeight = itCompromisedHeights->second.front();
//           if (fDebugRingSig)
//               for (auto i = itCompromisedHeights->second.size(); i-- > 0; )
//                   LogPrintf("CountAllAnonOutputs() : Compromised anon height value %d: index %d height %d\n",it->nValue, i, itCompromisedHeights->second.at(i));
        }
    }

    if (fDebugRingSig)
    {
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        LogPrintf("CountAllAnonOutputs(%d) - processed %d anons, %d keyImages in %d µs\n", nBlockHeight, nTotalAoRead, nTotalKiRead,
                  std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    }

    return 0;
};

int CWallet::CountOwnedAnonOutputs(std::map<int64_t, int>& mOwnedOutputCounts, MaturityFilter nFilter)
{
    if (fDebugRingSig)
        LogPrintf("CountOwnedAnonOutputs()\n");

    CWalletDB walletdb(strWalletFile, "r");

    Dbc* pcursor = walletdb.GetAtCursor();
    if (!pcursor)
        throw runtime_error("CWallet::CountOwnedAnonOutputs() : cannot create DB cursor");
    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("oao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("CWallet::CountOwnedAnonOutputs() : error scanning DB");
        };

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "oao")
            break;
        COwnedAnonOutput oao;
        ssKey >> oao.vchImage;

        ssValue >> oao;

        if (oao.fSpent)
            continue;

        WalletTxMap::iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi == mapWallet.end()
            || mi->second.nVersion != ANON_TXN_VERSION
            || mi->second.vout.size() <= oao.outpoint.n
            || mi->second.IsSpent(oao.outpoint.n))
            continue;

        //LogPrintf("[rem] mi->second.GetDepthInMainChain() %d \n", mi->second.GetDepthInMainChain());
        //LogPrintf("[rem] mi->second.hashBlock %s \n", mi->second.hashBlock.ToString().c_str());
        // -- check maturity
        if (nFilter != MaturityFilter::NONE)
        {
            // maturity (minDepth) depends on if the output was created in a staking transaction or is used for staking
            int minBlockHeight = mi->second.IsCoinStake() || nFilter == MaturityFilter::FOR_STAKING ?
                        Params().GetAnonStakeMinConfirmations() : MIN_ANON_SPEND_DEPTH;

            LOCK(cs_main);
            if (mi->second.GetDepthInMainChain() < minBlockHeight)
                continue;
        }

        // TODO: check ReadAnonOutput?

        oao.nValue = mi->second.vout[oao.outpoint.n].nValue;

        mOwnedOutputCounts[oao.nValue]++;
    };

    pcursor->close();
    return 0;
};

int CWallet::CountLockedAnonOutputs()
{
    if (fDebugRingSig)
    {
        LogPrintf("%s\n", __func__);
     };
    // -- count owned anon outputs received when wallet was locked.
    int result = 0;

    CWalletDB walletdb(strWalletFile, "cr+");
    Dbc *pcursor = walletdb.GetTxnCursor();
    if (!pcursor)
        throw runtime_error(strprintf("%s : cannot create DB cursor.", __func__).c_str());

    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("lao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        }
        else if (ret != 0)
        {
            pcursor->close();
            throw runtime_error(strprintf("%s : error scanning DB.", __func__).c_str());
        }
        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "lao")
            break;

        result++;
    }

    pcursor->close();
    return result;
}

uint64_t CWallet::EraseAllAnonData(std::function<void (const char *, const uint32_t&)> funcProgress)
{
    LogPrintf("EraseAllAnonData()\n");
    int64_t nStart = GetTimeMillis();

    LOCK2(cs_main, cs_wallet);
    CWalletDB walletdb(strWalletFile, "r+");
    CTxDB txdb("r+");

    uint32_t nAo = 0;
    uint32_t nKi = 0;

    LogPrintf("Erasing anon outputs.\n");
    if (funcProgress)
         txdb.EraseRange(std::string("ao"), nAo, [funcProgress] (const uint32_t& nErased) -> void {
             funcProgress("ATXO", nErased);
         });
    else
        txdb.EraseRange(std::string("ao"), nAo);
    LogPrintf("Erasing spent key images.\n");
    if (funcProgress)
         txdb.EraseRange(std::string("ki"), nKi, [funcProgress] (const uint32_t& nErased) -> void {
             funcProgress("key image", nErased);
         });
    else
        txdb.EraseRange(std::string("ki"), nKi);

    LogPrintf("Erasing compromised anon heights.\n");
    txdb.EraseCompromisedAnonHeights();

    uint32_t nLao = 0;
    uint32_t nOao = 0;
    uint32_t nOal = 0;
    uint32_t nOol = 0;

    LogPrintf("Erasing locked anon outputs.\n");
    walletdb.EraseRange(std::string("lao"), nLao);
    LogPrintf("Erasing owned anon outputs.\n");
    walletdb.EraseRange(std::string("oao"), nOao);
    LogPrintf("Erasing anon output links.\n");
    walletdb.EraseRange(std::string("oal"), nOal);
    LogPrintf("Erasing old output links.\n");
    walletdb.EraseRange(std::string("ool"), nOol);

    LogPrintf("EraseAllAnonData() Complete, %d %d %d %d %d %d, %15dms\n", nAo, nKi, nLao, nOao, nOal, nOol, GetTimeMillis() - nStart);

    mapAnonOutputStats.clear();
    mapAnonBlockStats.clear();

    return nAo + nKi + nLao + nOao + nOal + nOol;
};

bool CWallet::UpdateAnonStats(CTxDB& txdb, int nBlockHeight)
{
    if (fDebug)
        LogPrintf("UpdateAnonStats(%d)\n", nBlockHeight);

    AssertLockHeld(cs_main);

    // Update anon cache (mapAnonOutputStats) with block stats (mapAnonBlockStats)
    std::map<int64_t, CAnonBlockStat> & mapAnonBlockStat = mapAnonBlockStats[nBlockHeight];
    for (const auto & [nValue, anonBlockStat] : mapAnonBlockStat)
    {
        CAnonOutputCount& anonOutputCount = mapAnonOutputStats[nValue];
        uint16_t nNewAnons = anonBlockStat.nOutputs + anonBlockStat.nStakingOutputs;
        anonOutputCount.nSpends += anonBlockStat.nSpends;
        anonOutputCount.nExists += nNewAnons;
        anonOutputCount.nStakes += anonBlockStat.nStakingOutputs;
        if (nNewAnons > 0 && anonOutputCount.nLastHeight < nBlockHeight)
            anonOutputCount.nLastHeight = nBlockHeight;

        // Persist compromised anon block height in case all anons of one denomination has been spent
        if (anonOutputCount.nMature && anonBlockStat.nSpends && anonOutputCount.nMature - anonOutputCount.nSpends <= 0)
        {
            LogPrintf("%s: ALL SPENT of mature anon denomination %d in block height %d. -> persist compromised height.\n",
                      __func__, nValue, nBlockHeight);
            fStaleAnonCache = true;
            std::map<int64_t, std::vector<int>> mapCompromisedHeights;
            txdb.ReadCompromisedAnonHeights(mapCompromisedHeights);
            std::vector<int>& vCompromisedHeights = mapCompromisedHeights[nValue];
            if (std::find(vCompromisedHeights.begin(), vCompromisedHeights.end(), nBlockHeight) == vCompromisedHeights.end())
            {
                // find proper position in descending order
                std::vector<int>::iterator it = std::lower_bound(vCompromisedHeights.begin(), vCompromisedHeights.end(), nBlockHeight, std::greater<int>());
                vCompromisedHeights.insert(it, nBlockHeight); // insert before iterator it
                if (!txdb.WriteCompromisedAnonHeights(mapCompromisedHeights))
                    return error("%s: WriteCompromisedAnonHeights failed for anon value %s block height %d.", __func__, nValue, nBlockHeight);
            }
        }
    }

    if (fStaleAnonCache)
        return true;

    // -- Add outputs of new mature block to mapAnonBlockStats
    int nMatureHeight = nBlockHeight - MIN_ANON_SPEND_DEPTH + 1;
    if (nMatureHeight > 0 )
    {
        auto itBlockStatMature = mapAnonBlockStats.find(nMatureHeight);
        if (itBlockStatMature == mapAnonBlockStats.end())
        {
            LogPrintf("UpdateAnonStats(%d) - Missing stats for block %d => fStaleAnonCache = true\n", nBlockHeight, nMatureHeight);
            fStaleAnonCache = true;
            return true;
        }
        std::map<int64_t, CAnonBlockStat> mapBlockSpending = itBlockStatMature->second;
        for (const auto & [nValue, anonBlockStat] : mapBlockSpending)
        {
            uint16_t nMixinsUpdate = 0;
            if (mapAnonOutputStats[nValue].nCompromisedHeight - MIN_ANON_SPEND_DEPTH < nMatureHeight)
            {
                nMixinsUpdate = anonBlockStat.nOutputs - anonBlockStat.nCompromisedOutputs;
                mapAnonOutputStats[nValue].nMixins += nMixinsUpdate;
            }
            mapAnonOutputStats[nValue].nMature += anonBlockStat.nOutputs;
            if (fDebugRingSig)
                LogPrintf("UpdateAnonStats(%d) : [%d/%d] nMature+%d nMixins+%d\n",
                          nBlockHeight, nValue, nMatureHeight, anonBlockStat.nOutputs, nMixinsUpdate);
        }
    }

    // -- Add outputs of new stake mature block to mapAnonBlockStats
    int nMatureStakeHeight = nBlockHeight - Params().GetAnonStakeMinConfirmations() + 1;
    if (nMatureStakeHeight > 0)
    {
        auto itBlockStatStaking = mapAnonBlockStats.find(nMatureStakeHeight);
        if (itBlockStatStaking == mapAnonBlockStats.end())
        {
            LogPrintf("UpdateAnonStats(%d) - Missing stats for block %d => fStaleAnonCache = true\n", nBlockHeight, nMatureHeight);
            fStaleAnonCache = true;
            return true;
        }
        std::map<int64_t, CAnonBlockStat> mapBlockStaking = itBlockStatStaking->second;
        for (const auto & [nValue, anonBlockStat] : mapBlockStaking)
        {
            uint16_t nMixinsUpdate = 0, nMixinsStakingUpdate = 0;
            if (mapAnonOutputStats[nValue].nCompromisedHeight - MIN_ANON_SPEND_DEPTH < nMatureStakeHeight)
            {
                nMixinsUpdate = anonBlockStat.nStakingOutputs;
                nMixinsStakingUpdate = anonBlockStat.nStakingOutputs + anonBlockStat.nOutputs - anonBlockStat.nCompromisedOutputs;
                mapAnonOutputStats[nValue].nMixins += nMixinsUpdate;
                mapAnonOutputStats[nValue].nMixinsStaking += nMixinsStakingUpdate;
            }
            mapAnonOutputStats[nValue].nMature += anonBlockStat.nStakingOutputs;
            if (fDebugRingSig)
                LogPrintf("UpdateAnonStats(%d) : [%d/%d] nMature+%d nMixins+%d nMixinsStaking+%d\n",
                          nBlockHeight, nValue, nMatureStakeHeight, anonBlockStat.nStakingOutputs, nMixinsUpdate, nMixinsStakingUpdate);
        }
    }

    if (mapAnonBlockStats.size() > nMaxAnonBlockCache)
    {
        auto eraseIter = mapAnonBlockStats.begin();
        std::advance(eraseIter, mapAnonBlockStats.size() - nMaxAnonBlockCache);
        mapAnonBlockStats.erase(mapAnonBlockStats.begin(), eraseIter);
    }

    return true;
}

bool CWallet::RemoveAnonStats(CTxDB& txdb, int nBlockHeight)
{
    if (fDebugRingSig)
        LogPrintf("RemoveAnonStats(%d)\n", nBlockHeight);

    AssertLockHeld(cs_main);

    // check if compromised height (ALL SPENT) must be removed
    for (const auto & [nValue, anonOutputStat] : mapAnonOutputStats)
    {
        if (anonOutputStat.nCompromisedHeight >= nBlockHeight)
        {
            std::map<int64_t, std::vector<int>> mapCompromisedHeights;
            if (!txdb.ReadCompromisedAnonHeights(mapCompromisedHeights))
                return error("%s: ReadCompromisedAnonHeights failed on remove of block %d.", __func__, nBlockHeight);
            std::vector<int>& vCompromisedHeights = mapCompromisedHeights[nValue];
            vCompromisedHeights.erase(std::remove(vCompromisedHeights.begin(), vCompromisedHeights.end(), anonOutputStat.nCompromisedHeight), vCompromisedHeights.end());
            if (!txdb.WriteCompromisedAnonHeights(mapCompromisedHeights))
                return error("%s: WriteCompromisedAnonHeights failed on remove of block %d.", __func__, nBlockHeight);
            LogPrintf("RemoveAnonStats(%d) : Removed compromised block height %d for value %d\n", nBlockHeight, anonOutputStat.nCompromisedHeight, nValue);
            fStaleAnonCache = true; // force rebuild of anon cache
        }
    }

    if (fStaleAnonCache)
        return true;

    // -- Remove outputs of last mature block from mapAnonBlockStats
    int nMatureHeight = nBlockHeight - MIN_ANON_SPEND_DEPTH + 1;
    if (nMatureHeight > 0 )
    {
        auto itBlockStatMature = mapAnonBlockStats.find(nMatureHeight);
        if (itBlockStatMature == mapAnonBlockStats.end())
        {
            LogPrintf("RemoveAnonStats(%d) - Missing stats for block %d => fStaleAnonCache = true\n", nBlockHeight, nMatureHeight);
            fStaleAnonCache = true;
            return true;
        }
        std::map<int64_t, CAnonBlockStat> mapBlockSpending = itBlockStatMature->second;
        for (const auto & [nValue, anonBlockStat] : mapBlockSpending)
        {
            uint16_t nMixinsUpdate = 0;
            if (mapAnonOutputStats[nValue].nCompromisedHeight - MIN_ANON_SPEND_DEPTH < nMatureHeight)
            {
                nMixinsUpdate = anonBlockStat.nOutputs - anonBlockStat.nCompromisedOutputs;
                mapAnonOutputStats[nValue].nMixins -= nMixinsUpdate;
            }
            mapAnonOutputStats[nValue].nMature -= anonBlockStat.nOutputs;
            if (fDebugRingSig)
                LogPrintf("RemoveAnonStats(%d) : [%d/%d] nMature-%d nMixins-%d\n",
                          nBlockHeight, nValue, nMatureHeight, anonBlockStat.nOutputs, nMixinsUpdate);
        }
    }

    // -- Remove outputs of last stake mature block from mapAnonBlockStats
    int nMatureStakeHeight = nBlockHeight - Params().GetAnonStakeMinConfirmations() + 1;
    if (nMatureStakeHeight > 0)
    {
        auto itBlockStatStaking = mapAnonBlockStats.find(nMatureStakeHeight);
        if (itBlockStatStaking == mapAnonBlockStats.end())
        {
            LogPrintf("RemoveAnonStats(%d) - Missing stats for block %d => fStaleAnonCache = true\n", nBlockHeight, nMatureHeight);
            fStaleAnonCache = true;
            return true;
        }
        std::map<int64_t, CAnonBlockStat> mapBlockStaking = itBlockStatStaking->second;
        for (const auto & [nValue, anonBlockStat] : mapBlockStaking)
        {
            uint16_t nMixinsUpdate = 0, nMixinsStakingUpdate = 0;
            if (mapAnonOutputStats[nValue].nCompromisedHeight - MIN_ANON_SPEND_DEPTH < nMatureStakeHeight)
            {
                nMixinsUpdate = anonBlockStat.nStakingOutputs;
                nMixinsStakingUpdate = anonBlockStat.nStakingOutputs + anonBlockStat.nOutputs - anonBlockStat.nCompromisedOutputs;
                mapAnonOutputStats[nValue].nMixins -= nMixinsUpdate;
                mapAnonOutputStats[nValue].nMixinsStaking -= nMixinsStakingUpdate;
            }
            mapAnonOutputStats[nValue].nMature -= anonBlockStat.nStakingOutputs;
            if (fDebugRingSig)
                LogPrintf("RemoveAnonStats(%d) : [%d/%d] nMature-%d nMixins-%d nMixinsStaking-%d\n",
                          nBlockHeight, nValue, nMatureStakeHeight, anonBlockStat.nStakingOutputs, nMixinsUpdate, nMixinsStakingUpdate);
        }
    }

    mapAnonBlockStats.erase(nBlockHeight);

    return true;
}

bool CWallet::CacheAnonStats(int nBlockHeight, std::function<void (const unsigned mode, const uint32_t&)> funcProgress)
{
    if (fDebugRingSig)
        LogPrintf("CacheAnonStats(%d)\n", nBlockHeight);

    AssertLockHeld(cs_main);

    std::list<CAnonOutputCount> lOutputCounts;
    if (CountAllAnonOutputs(lOutputCounts, nBlockHeight, funcProgress) != 0)
    {
        LogPrintf("Error: CountAllAnonOutputs() failed.\n");
        return false;
    };

    mapAnonOutputStats.clear();
    // initialize stats from nMinTxFee to nMaxAnonOutput
    for (int64_t nValue = nMinTxFee; nValue <= nMaxAnonOutput; nValue *= 10)
    {
        mapAnonOutputStats[nValue].nValue = nValue;
        if (nValue < nMaxAnonOutput)
        {
            mapAnonOutputStats[nValue*3].nValue = nValue*3;
            mapAnonOutputStats[nValue*4].nValue = nValue*4;
            mapAnonOutputStats[nValue*5].nValue = nValue*5;
        }
    }

    for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
    {
        mapAnonOutputStats[it->nValue].set(
            it->nValue, it->nExists, it->nUnconfirmed, it->nSpends, it->nOwned,
            it->nLastHeight, it->nCompromised, it->nMature, it->nMixins, it->nMixinsStaking, it->nStakes, it->nCompromisedHeight); // mapAnonOutputStats stores height in chain instead of depth
    };
    fStaleAnonCache = false;

    return true;
};

bool CWallet::InitBloomFilter()
{
    if (fDebug)
        LogPrintf("Initialising bloom filter, max elements %d.\n", nBloomFilterElements);

    LOCK(cs_wallet);

    if (pBloomFilter)
        return error("Bloom filter already created.");

    char nFlags = BLOOM_UPDATE_ALL;

    if (nLocalRequirements & THIN_STEALTH)
        nFlags |= BLOOM_ACCEPT_STEALTH;
    pBloomFilter = new CBloomFilter(nBloomFilterElements, 0.001, GetRandUInt32(), nFlags);

    if (!pBloomFilter)
        return error("Bloom filter new failed.");

    if (!pBloomFilter->IsWithinSizeConstraints())
    {
        delete pBloomFilter;
        pBloomFilter = NULL;
        return error("Bloom filter is too large.");
    };

    // TODO: don't load addresses created from receiving stealth txns
    // TODO: exclude change addresses of spent outputs
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);

    uint32_t nKeysAdded = 0;

    // -- need to add change addresses too
    BOOST_FOREACH(const CKeyID &keyId, setKeys)
    {
        // -- don't add keys generated for aonon outputs (marked with label prefix "ao ")
        std::map<CTxDestination, std::string>::iterator mi(mapAddressBook.find(keyId));
        if (mi != mapAddressBook.end() && IsAnonMappingLabel(mi->second))
        {
            if (fDebugRingSig)
            {
                CBitcoinAddress coinAddress(keyId);
                LogPrintf("InitBloomFilter() - ignoring key for anon output %s.\n", coinAddress.ToString().c_str());
            };
            continue;
        };

        pBloomFilter->UpdateEmptyFull();
        if (pBloomFilter->IsFull())
        {
            // TODO: try resize?
            LogPrintf("Error: InitBloomFilter() - Filter is full.\n");
            continue; // continue so more messages show in log
        };

        if (fDebug)
        {
            CBitcoinAddress coinAddress(keyId);
            if (coinAddress.IsValid())
                LogPrintf("Adding key: %s.\n", coinAddress.ToString().c_str());
        };

        std::vector<unsigned char> vData(keyId.begin(), keyId.end());

        pBloomFilter->insert(vData);

        nKeysAdded++;
    };

    setKeys.clear();


    // - load unspent outputs
    uint32_t nOutPointsAdded = 0;

    for (WalletTxMap::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx& wtx = (*it).second;

        // -- add unspent outputs to bloom filters
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            if (wtx.nVersion == ANON_TXN_VERSION
                && txin.IsAnonInput())
                continue;

            WalletTxMap::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi == mapWallet.end())
                continue;

            CWalletTx& wtxPrev = (*mi).second;

            if (txin.prevout.n >= wtxPrev.vout.size())
            {
                LogPrintf("InitBloomFilter(): bad wtx %s\n", wtxPrev.GetHash().ToString().c_str());
            } else
            //if (!wtxPrev.IsSpent(txin.prevout.n) && IsMine(wtxPrev.vout[txin.prevout.n]))
            if (!wtxPrev.IsSpent(txin.prevout.n))
            {
                CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
                stream << txin.prevout;
                std::vector<unsigned char> vData(stream.begin(), stream.end());
                pBloomFilter->insert(vData);
                nOutPointsAdded++;
            };

        };
    };

    if (fDebug)
    {
        LogPrintf("Added %u key%s, %u outpoint%s to filter.\n",
            nKeysAdded, nKeysAdded != 1 ? "s": "",
            nOutPointsAdded, nOutPointsAdded != 1 ? "s": "");
        LogPrintf("Filter bytes: %u.\n", pBloomFilter->GetSize());
    };

    return true;
};


bool CWallet::IsMine(CStealthAddress stealthAddress)
{
    // - check legacy stealth addresses in wallet
    for (std::set<CStealthAddress>::iterator it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() != EC_SECRET_SIZE)
            continue; // stealth address in wallet is not owned

        if (it->scan_pubkey == stealthAddress.scan_pubkey && it->spend_pubkey != stealthAddress.spend_pubkey) {
            return true; // scan & spend public key match, we own this address
        }
    };

    // - check ext account stealth keys in wallet
    for ( ExtKeyAccountMap::const_iterator mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;

        for (AccStealthKeyMap::iterator it = ea->mapStealthKeys.begin(); it != ea->mapStealthKeys.end(); ++it)
        {
            const CEKAStealthKey &aks = it->second;

            if (!aks.skScan.IsValid())
                continue; // stealth address in wallet is not valid

            if (aks.pkScan == stealthAddress.scan_pubkey && aks.pkSpend == stealthAddress.spend_pubkey) {
                return true; // scan & spend public key match, we own this address
            }
        };
    };
    return false;
}



// NovaCoin: get current stake weight PoSv2 or PoSv3
uint64_t CWallet::GetStakeWeight() const
{
    int64_t nCurrentTime = GetAdjustedTime();
    uint64_t nWeight = 0;

    // -- Get XSPEC weight for staking
    int64_t nBalance = GetBalance();
    if (nBalance > nReserveBalance)
    {
        set<pair<const CWalletTx*,unsigned int> > setCoins;
        int64_t nValueIn = 0;
        if (SelectCoinsForStaking(nBalance - nReserveBalance, nCurrentTime, setCoins, nValueIn))
            nWeight += nValueIn;
    }

    return nWeight;
}

uint64_t CWallet::GetSpectreStakeWeight() const
{
    int64_t nCurrentTime = GetAdjustedTime();
    uint64_t nWeight = 0;

    // -- Get SPECTRE weight for staking
    if (Params().IsForkV3(nCurrentTime))
    {
        int64_t nSpectreBalance = GetSpectreBalance();
        if (nSpectreBalance > nReserveBalance)
        {
            std::list<COwnedAnonOutput> lAvailableCoins;
            int64_t nAmountCheck = 0;
            std::string sError;
            if (ListAvailableAnonOutputs(lAvailableCoins, nAmountCheck, MIN_RING_SIZE, MaturityFilter::FOR_STAKING, sError, nSpectreBalance - nReserveBalance))
                nWeight += nAmountCheck;
        }
    }

    return nWeight;
}

boost::random::mt19937 stakingDonationRng;
boost::random::uniform_int_distribution<> stakingDonationDistribution(0, 99);

bool CWallet::CreateCoinStake(unsigned int nBits, int64_t nSearchInterval, int64_t nFees, CTransaction& txNew, CKey& key)
{
    CBlockIndex* pindexPrev = pindexBest;
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    // Choose coins to use
    int64_t nBalance = GetBalance();

    if (nBalance <= nReserveBalance)
        return false;

    std::vector<const CWalletTx*> vwtxPrev;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;

    // Select coins with suitable depth
    if (!SelectCoinsForStaking(nBalance - nReserveBalance, txNew.nTime, setCoins, nValueIn))
        return false;

    if (setCoins.empty())
        return false;

    int64_t nCredit = 0;
    CScript scriptPubKeyKernel;
    CTxDB txdb("r");
    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        boost::this_thread::interruption_point();
        static int nMaxStakeSearchInterval = 60;

        bool fKernelFound = false;
        for (unsigned int n=0; n<min(nSearchInterval,(int64_t)nMaxStakeSearchInterval) && !fKernelFound && pindexPrev == pindexBest; n++)
        {
            boost::this_thread::interruption_point();
            // Search backward in time from the given txNew timestamp
            // Search nSearchInterval seconds back up to nMaxStakeSearchInterval
            COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);

            int64_t nBlockTime;
            if (CheckKernel(pindexPrev, nBits, txNew.nTime - n, prevoutStake, &nBlockTime))
            {
                // Found a kernel
                if (fDebugPoS)
                    LogPrintf("CreateCoinStake : kernel found\n");

                std::vector<valtype> vSolutions;
                txnouttype whichType;
                CScript scriptPubKeyOut;
                scriptPubKeyKernel = pcoin.first->vout[pcoin.second].scriptPubKey;

                if (!Solver(scriptPubKeyKernel, whichType, vSolutions))
                {
                    if (fDebugPoS)
                        LogPrintf("CreateCoinStake : failed to parse kernel\n");
                    break;
                };

                if (fDebugPoS)
                    LogPrintf("CreateCoinStake : parsed kernel type=%d\n", whichType);

                if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
                {
                    if (fDebugPoS)
                        LogPrintf("CreateCoinStake : no support for kernel type=%d\n", whichType);
                    break;  // only support pay to public key and pay to address
                };

                if (whichType == TX_PUBKEYHASH) // pay to address type
                {
                    // convert to pay to public key type
                    if (!GetKey(uint160(vSolutions[0]), key))
                    {
                        if (fDebugPoS)
                            LogPrintf("CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    };
                    scriptPubKeyOut << key.GetPubKey() << OP_CHECKSIG;
                };

                if (whichType == TX_PUBKEY)
                {
                    valtype& vchPubKey = vSolutions[0];
                    if (!GetKey(Hash160(vchPubKey), key))
                    {
                        if (fDebugPoS)
                            LogPrintf("CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    };

                    if (key.GetPubKey() != vchPubKey)
                    {
                        if (fDebugPoS)
                            LogPrintf("CreateCoinStake : invalid key for kernel type=%d\n", whichType);
                        break; // keys mismatch
                    };

                    scriptPubKeyOut = scriptPubKeyKernel;
                };

                txNew.nTime -= n;
                txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
                nCredit += pcoin.first->vout[pcoin.second].nValue;
                vwtxPrev.push_back(pcoin.first);
                txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));

                if (fDebugPoS)
                    LogPrintf("CreateCoinStake : added kernel type=%d\n", whichType);
                fKernelFound = true;
                break;
            };
        };

        if (fKernelFound)
            break; // if kernel is found stop searching
    }

    if (nCredit == 0 || nCredit > nBalance - nReserveBalance)
        return false;

    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        // Attempt to add more inputs
        // Only add coins of the same key/address as kernel
        if (txNew.vout.size() == 2 && ((pcoin.first->vout[pcoin.second].scriptPubKey == scriptPubKeyKernel || pcoin.first->vout[pcoin.second].scriptPubKey == txNew.vout[1].scriptPubKey))
            && pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
        {
            int64_t nTimeWeight = GetWeight((int64_t)pcoin.first->nTime, (int64_t)txNew.nTime);

            // Stop adding more inputs if already too many inputs
            if (txNew.vin.size() >= 100)
                break;
            // Stop adding more inputs if value is already pretty significant
            if (nCredit >= nStakeCombineThreshold)
                break;
            // Stop adding inputs if reached reserve limit
            if (nCredit + pcoin.first->vout[pcoin.second].nValue > nBalance - nReserveBalance)
                break;
            // Do not add additional significant input
            if (pcoin.first->vout[pcoin.second].nValue >= nStakeCombineThreshold)
                continue;

            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            nCredit += pcoin.first->vout[pcoin.second].nValue;
            vwtxPrev.push_back(pcoin.first);
        };
    };

    // Calculate coin age reward
    int64_t nReward;
    {
        uint64_t nCoinAge = 0;
        CTxDB txdb("r");
        if (!Params().IsProtocolV3(pindexPrev->nHeight) && !txNew.GetCoinAge(txdb, pindexPrev, nCoinAge))
            return error("CreateCoinStake : failed to calculate coin age");

        nReward = Params().GetProofOfStakeReward(pindexPrev, nCoinAge, nFees);
        if (nReward <= 0)
            return false;

        nCredit += nReward;
    }

    if (nCredit >= nStakeSplitThreshold)
        txNew.vout.push_back(CTxOut(0, txNew.vout[1].scriptPubKey)); //split stake

    // Set output amount
    if (txNew.vout.size() == 3)
    {
        txNew.vout[1].nValue = (nCredit / 2 / CENT) * CENT;
        txNew.vout[2].nValue = nCredit - txNew.vout[1].nValue;
    } else
        txNew.vout[1].nValue = nCredit;

    // (Possibly) donate the stake to developers, according to the configured probability
    int sample = stakingDonationDistribution(stakingDonationRng);
    LogPrintf("sample: %d, donation: %d\n", sample, nStakingDonation);
    bool fSupplyIncrease = Params().IsForkV4SupplyIncrease(pindexPrev);
    if (fSupplyIncrease || sample < nStakingDonation || (pindexPrev->nHeight+1) % 6 == 0) {
        LogPrintf("Donating this (potential) stake to the developers\n");
        CBitcoinAddress address(fSupplyIncrease ? Params().GetSupplyIncreaseAddress() : Params().GetDevContributionAddress());
        int64_t reduction = nReward;
        // reduce outputs popping as necessary until we've reduced by nReward
        if (txNew.vout.size() == 3) {
            LogPrintf("donating a split stake\n");
            if (txNew.vout[2].nValue <= reduction) {
                // The second part of the split stake was less than or equal to the
                // amount we need to reduce by, so we need to un-split the stake.
                reduction -= txNew.vout[2].nValue;
                txNew.vout.pop_back();
                LogPrintf("undid splitting of stake due to donation exceeding second output size\n");
            }
            else {
                txNew.vout[2].nValue -= reduction;
                reduction = 0;
                LogPrintf("successfully took donation from second output of split stake\n");
            }
        }
        if (reduction > 0) {
            if (txNew.vout[1].nValue <= reduction) {
                LogPrintf("Total of stake outputs was less than expected credit. Bailing out\n");
                return false;
            }
            txNew.vout[1].nValue -= reduction;
        }
        // push a new output donating to the developers
        CScript script;
        script.SetDestination(address.Get());
        txNew.vout.push_back(CTxOut(nReward, script));
        LogPrintf("donation complete\n");
    }
    else {
        LogPrintf("Not donating this (potential) stake to the developers\n");
    }

    // Sign
    int nIn = 0;
    BOOST_FOREACH(const CWalletTx* pcoin, vwtxPrev)
    {
        if (!SignSignature(*this, *pcoin, txNew, nIn++))
            return error("CreateCoinStake : failed to sign coinstake");
    };

    // Limit size
    unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
    if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
        return error("CreateCoinStake : exceeded coinstake size limit");

    // Successfully generated coinstake
    return true;
}


bool CWallet::CreateAnonCoinStake(unsigned int nBits, int64_t nSearchInterval, int64_t nFees, CTransaction& txNew, CKey& key)
{
    // Ring size for staking is MIN_RING_SIZE
    int nRingSize = MIN_RING_SIZE;

    CBlockIndex* pindexPrev = pindexBest;
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    // Choose coins to use
    int64_t nBalance = GetSpectreBalance();
    if (nBalance <= nReserveBalance)
        return false;

    // -------------------------------------------
    // Select coins with suitable depth
    std::list<COwnedAnonOutput> lAvailableCoins;
    int64_t nAmountCheck;
    std::string sError;
    if (!ListAvailableAnonOutputs(lAvailableCoins, nAmountCheck, nRingSize, MaturityFilter::FOR_STAKING, sError, nBalance - nReserveBalance))
        return error(("CreateAnonCoinStake : " + sError).c_str());
    if (lAvailableCoins.empty())
        return false;

    bool fKernelFound = false;
    for (const auto & oao : lAvailableCoins)
    {
        boost::this_thread::interruption_point();

        static int nMaxStakeSearchInterval = 60;
        for (unsigned int n=0; n<min(nSearchInterval,(int64_t)nMaxStakeSearchInterval) && !fKernelFound && pindexPrev == pindexBest; n++)
        {
            boost::this_thread::interruption_point();
            // Search backward in time from the given txNew timestamp
            // Search nSearchInterval seconds back up to nMaxStakeSearchInterval
            if (CheckAnonKernel(pindexPrev, nBits, oao.nValue, oao.vchImage, txNew.nTime - n))
            {
                // Found a kernel
                if (fDebugPoS)
                    LogPrintf("CreateAnonCoinStake : kernel found for keyImage %s\n", HexStr(oao.vchImage));

                LOCK(cs_main);

                txNew.nVersion = ANON_TXN_VERSION;
                txNew.nTime -= n;

                int64_t nCredit = 0;
                std::vector<const COwnedAnonOutput*> vPickedCoins;
                vPickedCoins.push_back(&oao);

                // -- Check if stake should be split for balancing unspent ATXOs
                std::vector<int64_t> vOutAmounts;
                // find the anon denomination with the least number of unspent outputs
                CAnonOutputCount* lowestAOC = nullptr;
                for (auto it = mapAnonOutputStats.rbegin(); it != mapAnonOutputStats.rend(); it++)
                {
                    if (it->first < nMinTxFee * 10)
                        break;
                    if (it->first >= oao.nValue)
                        continue;
                    if (it->second.numOfUnspends() > UNSPENT_ANON_BALANCE_MIN)
                        continue;
                    if (lowestAOC == nullptr || lowestAOC->numOfUnspends() > it->second.numOfUnspends())
                        lowestAOC = &it->second;
                }
                if (lowestAOC)
                {
                    vOutAmounts.push_back(lowestAOC->nValue);
                    nCredit += oao.nValue - lowestAOC->nValue;
                    LogPrintf("CreateAnonCoinStake : Split anon stake of value %d to create 1 additional ATXO of value %d which has only %d unspents\n",
                              oao.nValue, lowestAOC->nValue, lowestAOC->numOfUnspends());
                }
                else
                    nCredit += oao.nValue;

                // -- Add more anon inputs for consolidation
                int64_t nMaxCombineOutput = nMaxAnonStakeOutput / 10;
                int64_t lastCombineValue = -1;
                std::vector<const COwnedAnonOutput*> vConsolidateCoins;
                int nMaxConsolidation = 0, nNumOfConsolidated = 0;
                for (const auto & oaoc : lAvailableCoins)
                {
                    if (oaoc.nValue > nMaxCombineOutput)
                        break;
                    // skip the input used for staking (TODO could be optimized by considering in combining inputs)
                    if (&oaoc == &oao)
                        continue;
                    if (lastCombineValue != oaoc.nValue)
                    {
                        vConsolidateCoins.clear();
                        lastCombineValue = oaoc.nValue;
                        // calculate how many outputs can be consolidated considering the amount of mature unspents
                        nMaxConsolidation = mapAnonOutputStats[oaoc.nValue].numOfMatureUnspends() - UNSPENT_ANON_BALANCE_MAX;
                        nNumOfConsolidated = 0;
                    }
                    vConsolidateCoins.push_back(&oaoc);
                    nNumOfConsolidated++;
                    if (nNumOfConsolidated <= nMaxConsolidation && vConsolidateCoins.size() == 10)
                    {
                        vPickedCoins.insert(vPickedCoins.end(), vConsolidateCoins.begin(), vConsolidateCoins.end());
                        vConsolidateCoins.clear();
                        nCredit += oaoc.nValue * 10;
                        LogPrintf("CreateAnonCoinStake : Consolidate 10 additional ATXOs of value %d which has %d mature unspents\n",
                                  oaoc.nValue, mapAnonOutputStats[oaoc.nValue].numOfMatureUnspends());
                    }
                    // Consolidate maximal 50 inputs
                    if (vPickedCoins.size() == 51)
                        break;
                }

                // -- Calculate staking reward
                int64_t nReward = Params().GetProofOfAnonStakeReward(pindexPrev, nFees);
                if (nReward <= 0)
                    return error("CreateAnonCoinStake : GetProofOfStakeReward() reward <= 0");

                // -- Check if staking reward gets donated to developers, according to the configured probability and DCB rules
                int sample = stakingDonationDistribution(stakingDonationRng);
                LogPrintf("sample: %d, donation: %d\n", sample, nStakingDonation);
                bool donateReward = false;
                bool fSupplyIncrease = Params().IsForkV4SupplyIncrease(pindexPrev);
                if (fSupplyIncrease || sample < nStakingDonation || (pindexPrev->nHeight+1) % 6 == 0) {
                    LogPrintf("Donating this (potential) stake to the developers\n");
                    donateReward = true;
                }
                else {
                    LogPrintf("Not donating this (potential) stake to the developers\n");
                    nCredit += nReward;
                }

                // -- Get stealth address for creating new anon outputs.
                CStealthAddress sxAddress;
                if (!GetAnonStakeAddress(oao, sxAddress))
                    return error("CreateAnonCoinStake : GetAnonStakeAddress() change failed");

                // -- create anon output
                CScript scriptNarration; // needed to match output id of narr
                std::vector<std::pair<CScript, int64_t> > vecSend;
                std::vector<ec_secret> vecSecShared;
                std::string sNarr;
                if (nCredit)
                    splitAmount(nCredit, vOutAmounts, nMaxAnonStakeOutput);
                if (!CreateAnonOutputs(&sxAddress, vOutAmounts, sNarr, vecSend, scriptNarration, nullptr, &vecSecShared))
                    return error("CreateAnonCoinStake : CreateAnonOutputs() failed");

                // Sort anon ouputs together with corresponding ec_secret ascending by anon value
                std::vector<std::pair<CTxOut, ec_secret>> vTxOutSecret;
                vTxOutSecret.reserve(vecSend.size());
                for (uint32_t i = 0; i < vecSend.size(); ++i)
                    vTxOutSecret.push_back(std::make_pair(CTxOut(vecSend.at(i).second, vecSend.at(i).first), vecSecShared.at(i)));
                std::sort(vTxOutSecret.begin(), vTxOutSecret.end(), [] (const auto &a, const auto &b) {
                    return (a.first < b.first);
                });
                // Add sorted anon outputs to transaction
                for (auto [txOut, secret] : vTxOutSecret)
                    txNew.vout.push_back(txOut);

                // -- Set one-time private key of vout[1] for signing the block
                ec_secret sSpend;
                ec_secret sSpendR;
                memcpy(&sSpend.e[0], &sxAddress.spend_secret[0], EC_SECRET_SIZE);
                if (StealthSharedToSecretSpend(vTxOutSecret.at(0).second, sSpend, sSpendR) != 0)
                    return error("CreateAnonCoinStake : failed to get private key of anon output");
                key.Set(&sSpendR.e[0], true);

                // -- create donation output
                if (donateReward)
                {
                    CBitcoinAddress address(fSupplyIncrease ? Params().GetSupplyIncreaseAddress() : Params().GetDevContributionAddress());
                    // push a new output donating to the developers
                    CScript script;
                    script.SetDestination(address.Get());
                    txNew.vout.push_back(CTxOut(nReward, script));
                    LogPrintf("donation complete\n");
                }

                // -- create anon inputs
                txNew.vin.resize(vPickedCoins.size());
                uint256 preimage = 0; // not needed for RING_SIG_2
                uint32_t iVin = 0;
                // Initialize mixins set
                CMixins mixins;
                if (!InitMixins(mixins, vPickedCoins, true))
                     return error("CreateAnonCoinStake() : InitMixins() failed");

                for (const auto * pickedCoin : vPickedCoins)
                {
                    int oaoRingIndex;
                    if (!AddAnonInput(mixins, txNew.vin[iVin], *pickedCoin, RING_SIG_2, nRingSize, oaoRingIndex, true, false, sError))
                        return error(("CreateAnonCoinStake() : " + sError).c_str());

                    if (!GenerateRingSignature(txNew.vin[iVin], RING_SIG_2, nRingSize, oaoRingIndex, preimage, sError))
                        return error(("CreateAnonCoinStake() : " + sError).c_str());

                    iVin++;
                }

                // -- check if new coins already exist (in case random is broken ?)
                if (!AreOutputsUnique(txNew))
                    return error("CreateAnonCoinStake() : anon outputs are not unique - is random working?!");

                if (fDebugPoS)
                    LogPrintf("CreateAnonCoinStake() : added kernel for keyImage %s\n", HexStr(oao.vchImage));

                fKernelFound = true;
                break;
            }
        }

        if (fKernelFound)
            break; // if kernel is found stop searching
    }

    if (!fKernelFound)
        return false;

    // Limit size
    unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
    if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
        return error("CreateAnonCoinStake() : exceeded coinstake size limit");

    // Successfully generated coinstake
    return true;
}


// Call after CreateTransaction unless you want to abort
bool CWallet::CommitTransaction(CWalletTx& wtxNew, const std::map<CKeyID, CStealthAddress> * const mapPubStealth)
{
    if (!wtxNew.CheckTransaction())
    {
        LogPrintf("%s: CheckTransaction() failed %s.\n", __func__, wtxNew.GetHash().ToString().c_str());
        return false;
    };

    mapValue_t mapNarr;
    FindStealthTransactions(wtxNew, mapNarr);

    bool fIsMine = false;
    if (wtxNew.nVersion == ANON_TXN_VERSION)
    {
        LOCK2(cs_main, cs_wallet);
        CWalletDB walletdb(strWalletFile, "cr+");
        CTxDB txdb("cr+");

        walletdb.TxnBegin();
        txdb.TxnBegin();
        std::vector<WalletTxMap::iterator> vUpdatedTxns;
        std::map<int64_t, CAnonBlockStat> mapAnonBlockStat;
        if (!ProcessAnonTransaction(&walletdb, &txdb, wtxNew, wtxNew.hashBlock, fIsMine, mapNarr, vUpdatedTxns, mapAnonBlockStat, mapPubStealth))
        {
            LogPrintf("%s: ProcessAnonTransaction() failed %s.\n", __func__, wtxNew.GetHash().ToString().c_str());
            walletdb.TxnAbort();
            txdb.TxnAbort();
            // TODO erase keyImages in mempool
            return false;
        } else
        {
            walletdb.TxnCommit();
            txdb.TxnCommit();
            for (std::vector<WalletTxMap::iterator>::iterator it = vUpdatedTxns.begin();
                it != vUpdatedTxns.end(); ++it)
                NotifyTransactionChanged(this, (*it)->first, CT_UPDATED);
        };
    };

    if (!mapNarr.empty())
    {
        BOOST_FOREACH(const PAIRTYPE(string,string)& item, mapNarr)
            wtxNew.mapValue[item.first] = item.second;
    };

    {
        LOCK2(cs_main, cs_wallet);
        LogPrintf("%s:\n%s", __func__, wtxNew.ToString().c_str());

        {
            // This is only to keep the database open to defeat the auto-flush for the
            // duration of this scope.  This is the only place where this optimization
            // maybe makes sense; please don't do it anywhere else.
            CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r") : NULL;

            // Take key pair from key pool so it won't be used again
            //reservekey.KeepKey(); // [rm]

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            uint256 hash = wtxNew.GetHash();
            AddToWallet(wtxNew, hash);

            // Mark old coins as spent
            set<CWalletTx*> setCoins;
            BOOST_FOREACH(const CTxIn& txin, wtxNew.vin)
            {
                if (wtxNew.nVersion == ANON_TXN_VERSION
                    && txin.IsAnonInput())
                    continue;
                CWalletTx &coin = mapWallet[txin.prevout.hash];
                coin.BindWallet(this);
                coin.MarkSpent(txin.prevout.n);
                coin.WriteToDisk();
                NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
            };

            if (fFileBacked)
                delete pwalletdb;
        }

        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;

        // Broadcast
        if (!wtxNew.AcceptToMemoryPool())
        {
            // This must not fail. The transaction has already been signed and recorded.
            LogPrintf("%s: Error: Transaction not valid.\n", __func__);
            return false;
        };
        wtxNew.RelayWalletTransaction();


        // - look for new change addresses
        BOOST_FOREACH(CTxOut txout, wtxNew.vout)
        {
            if (wtxNew.nVersion == ANON_TXN_VERSION
                && txout.IsAnonOutput())
                continue;

            if (IsChange(txout))
            {
                CTxDestination txoutAddr;
                if (!ExtractDestination(txout.scriptPubKey, txoutAddr))
                    continue;
                if (pBloomFilter)
                    AddKeyToMerkleFilters(txoutAddr);
            };
        };

    } // cs_main, cs_wallet
    return true;
}




std::string CWallet::SendMoney(CScript scriptPubKey, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee)
{
    int64_t nFeeRequired;

    if (IsLocked())
    {
        std::string strError = _("Error: Wallet locked, unable to create transaction  ");
        LogPrintf("SendMoney() : %s", strError.c_str());
        return strError;
    };

    if (fWalletUnlockStakingOnly)
    {
        std::string strError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        LogPrintf("SendMoney() : %s", strError.c_str());
        return strError;
    };

    if (!CreateTransaction(scriptPubKey, nValue, sNarr, wtxNew, nFeeRequired))
    {
        std::string strError;
        if (nValue + nFeeRequired > GetBalance())
            strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
        else
            strError = _("Error: Transaction creation failed  ");
        LogPrintf("SendMoney() : %s", strError.c_str());
        return strError;
    };

    if (fAskFee && !uiInterface.ThreadSafeAskFee(nFeeRequired, _("Sending...")))
        return "ABORTED";

    if (!CommitTransaction(wtxNew))
        return _("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    return "";
}



std::string CWallet::SendMoneyToDestination(const CTxDestination& address, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee)
{
    // Check amount
    if (nValue <= 0)
        return _("Invalid amount");
    if (nValue + nTransactionFee > GetBalance())
        return _("Insufficient funds");

    if (sNarr.length() > 24)
        return _("Note must be 24 characters or less.");

    // Parse Bitcoin address
    CScript scriptPubKey;
    uint32_t nChildKey;
    CExtKeyPair ek;
    if (address.type() == typeid(CExtKeyPair))
    {
        ek = boost::get<CExtKeyPair>(address);
        CExtKey58 ek58;
        ek58.SetKeyP(ek);
        if (0 != ExtKeyGetDestination(ek, scriptPubKey, nChildKey))
            return "ExtKeyGetDestination failed.";
    } else
        scriptPubKey.SetDestination(address);

    std::string rv = SendMoney(scriptPubKey, nValue, sNarr, wtxNew, fAskFee);

    if (address.type() == typeid(CExtKeyPair) && rv == "")
        ExtKeyUpdateLooseKey(ek, nChildKey, true);

    return rv;
};




DBErrors CWallet::LoadWallet(int& oltWalletVersion, std::function<void (const uint32_t&)> funcProgress)
{
    if (!fFileBacked)
        return DB_LOAD_OK;

    DBErrors nLoadWalletRet = CWalletDB(strWalletFile,"cr+").LoadWallet(this, oltWalletVersion, funcProgress);
    if (nLoadWalletRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // the requires a new key.
        };
    };

    if (nLoadWalletRet != DB_LOAD_OK)
        return nLoadWalletRet;
    return DB_LOAD_OK;
}


bool CWallet::SetAddressBookName(const CTxDestination& address, const string& strName, CWalletDB *pwdb, bool fAddKeyToMerkleFilters, bool fManual)
{
    bool fOwned;
    ChangeType nMode;

    {
        LOCK(cs_wallet); // mapAddressBook
        std::map<CTxDestination, std::string>::iterator mi = mapAddressBook.find(address);
        nMode = (mi == mapAddressBook.end()) ? CT_NEW : CT_UPDATED;
        fOwned = IsDestMine(*this, address);

        mapAddressBook[address] = strName;
    }

    // -- fAddKeyToMerkleFilters is always false when adding keys for anonoutputs
    if (nMode == CT_NEW
        && pBloomFilter
        && fAddKeyToMerkleFilters)
    {
        AddKeyToMerkleFilters(address);
    };

    NotifyAddressBookChanged(this, address, strName, fOwned, nMode, fManual);

    if (!fFileBacked)
        return false;

    if (!pwdb)
        return CWalletDB(strWalletFile).WriteName(CBitcoinAddress(address).ToString(), strName);
    return pwdb->WriteName(CBitcoinAddress(address).ToString(), strName);
}

bool CWallet::DelAddressBookName(const CTxDestination& address, CWalletDB *pwdb)
{
    if (address.type() == typeid(CStealthAddress))
    {
        const CStealthAddress& sxAddr = boost::get<CStealthAddress>(address);
        bool fOwned; // must check on copy from wallet

        {
            LOCK(cs_wallet);

            std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxAddr);
            if (si == stealthAddresses.end())
            {
                LogPrintf("Error: DelAddressBookName() stealth address not found in wallet.\n");
                return false;
            };

            fOwned = si->scan_secret.size() < 32 ? false : true;

            if (stealthAddresses.erase(sxAddr) < 1
                || !CWalletDB(strWalletFile).EraseStealthAddress(sxAddr))
            {
                LogPrintf("Error: DelAddressBookName() remove stealthAddresses failed.\n");
                return false;
            };
        }

        NotifyAddressBookChanged(this, address, "", fOwned, CT_DELETED, true);
        return true;
    };


    {
        LOCK(cs_wallet); // mapAddressBook

        mapAddressBook.erase(address);
    }

    bool fOwned = IsDestMine(*this, address);
    string sName = "";

    NotifyAddressBookChanged(this, address, "", fOwned, CT_DELETED, true);

    if (!fFileBacked)
        return false;

    if (!pwdb)
        return CWalletDB(strWalletFile).EraseName(CBitcoinAddress(address).ToString());
    return pwdb->EraseName(CBitcoinAddress(address).ToString());
}


void CWallet::PrintWallet(const CBlock& block)
{
    {
        LOCK(cs_wallet);
        if (block.IsProofOfWork() && mapWallet.count(block.vtx[0].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[0].GetHash()];
            LogPrintf("    mine:  %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
        };

        if (block.IsProofOfStake() && mapWallet.count(block.vtx[1].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[1].GetHash()];
            LogPrintf("    stake: %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
        };

    }
    LogPrintf("\n");
}

bool CWallet::GetTransaction(const uint256 &hashTx, CWalletTx& wtx)
{
    {
        LOCK(cs_wallet);
        WalletTxMap::iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
        {
            wtx = (*mi).second;
            return true;
        };
    }
    return false;
}

bool CWallet::SetDefaultKey(const CPubKey &vchPubKey)
{
    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
            return false;
    };
    vchDefaultKey = vchPubKey;
    return true;
}

bool GetWalletFile(CWallet* pwallet, string &strWalletFileOut)
{
    if (!pwallet->fFileBacked)
        return false;
    strWalletFileOut = pwallet->strWalletFile;
    return true;
}

//
// Mark old keypool keys as used,
// and generate all new keys
//
bool CWallet::NewKeyPool()
{
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);
        BOOST_FOREACH(int64_t nIndex, setKeyPool)
            walletdb.ErasePool(nIndex);
        setKeyPool.clear();

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", 100), (int64_t)0);
        for (int i = 0; i < nKeys; i++)
        {
            int64_t nIndex = i+1;
            walletdb.WritePool(nIndex, CKeyPool(GenerateNewKey()));
            setKeyPool.insert(nIndex);
        };
        LogPrintf("CWallet::NewKeyPool wrote %d new keys\n", nKeys);
    }
    return true;
}

bool CWallet::TopUpKeyPool(unsigned int nSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        unsigned int nTargetSize;
        if (nSize > 0)
            nTargetSize = nSize;
        else
            nTargetSize = max(GetArg("-keypool", 100), (int64_t)0);

        while (setKeyPool.size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!setKeyPool.empty())
                nEnd = *(--setKeyPool.end()) + 1;
            if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                throw runtime_error("TopUpKeyPool() : writing generated key failed");
            setKeyPool.insert(nEnd);
            LogPrintf("keypool added key %d, size=%u\n", nEnd, setKeyPool.size());
        };
    }
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
{
    assert(false);
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpKeyPool();

        // Get the oldest key
        if (setKeyPool.empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(setKeyPool.begin());
        setKeyPool.erase(setKeyPool.begin());
        if (!walletdb.ReadPool(nIndex, keypool))
            throw runtime_error("ReserveKeyFromKeyPool() : read failed");
        if (!HaveKey(keypool.vchPubKey.GetID()))
            throw runtime_error("ReserveKeyFromKeyPool() : unknown key in key pool");
        assert(keypool.vchPubKey.IsValid());
        if (fDebug && GetBoolArg("-printkeypool"))
            LogPrintf("keypool reserve %d\n", nIndex);
    }
}

int64_t CWallet::AddReserveKey(const CKeyPool& keypool)
{
    {
        LOCK2(cs_main, cs_wallet);
        CWalletDB walletdb(strWalletFile);

        int64_t nIndex = 1 + *(--setKeyPool.end());
        if (!walletdb.WritePool(nIndex, keypool))
            throw runtime_error("AddReserveKey() : writing added key failed");
        setKeyPool.insert(nIndex);
        return nIndex;
    }
    return -1;
}

void CWallet::KeepKey(int64_t nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.ErasePool(nIndex);
    };

    if (fDebug)
        LogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex)
{
    assert(false); // [rm]

    // Return to key pool
    {
        LOCK(cs_wallet);
        setKeyPool.insert(nIndex);
    }
    if (fDebug)
        LogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result, bool fAllowReuse)
{
    int64_t nIndex = 0;
    CKeyPool keypool;

    assert(false); // replace with HD

    {
        LOCK(cs_wallet);
        ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
        {
            if (fAllowReuse && vchDefaultKey.IsValid())
            {
                result = vchDefaultKey;
                return true;
            };

            if (IsLocked())
                return false;

            result = GenerateNewKey();
            return true;
        };
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    int64_t nIndex = 0;
    CKeyPool keypool;
    ReserveKeyFromKeyPool(nIndex, keypool);
    if (nIndex == -1)
        return GetTime();
    ReturnKey(nIndex);
    return keypool.nTime;
}

std::map<CTxDestination, int64_t> CWallet::GetAddressBalances()
{
    std::map<CTxDestination, int64_t> balances;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
        {
            CWalletTx *pcoin = &walletEntry.second;

            if (!pcoin->IsFinal() || !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < (pcoin->IsFromMe() ? 0 : 1))
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, addr))
                    continue;

                int64_t n = pcoin->IsSpent(i) ? 0 : pcoin->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

std::set<std::set<CTxDestination> > CWallet::GetAddressGroupings()
{
    AssertLockHeld(cs_wallet); // mapWallet
    set< set<CTxDestination> > groupings;
    set<CTxDestination> grouping;

    BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
    {
        CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->vin.size() > 0 && IsMine(pcoin->vin[0]))
        {
            // group all input addresses with each other
            BOOST_FOREACH(CTxIn txin, pcoin->vin)
            {
                CTxDestination address;
                if(!ExtractDestination(mapWallet[txin.prevout.hash].vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
            }

            // group change with input addresses
            BOOST_FOREACH(CTxOut txout, pcoin->vout)
            {
                if (IsChange(txout))
                {
                    CWalletTx tx = mapWallet[pcoin->vin[0].prevout.hash];
                    CTxDestination txoutAddr;
                    if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                        continue;
                    grouping.insert(txoutAddr);
                };
            };
            groupings.insert(grouping);
            grouping.clear();
        };

        // group lone addrs by themselves
        for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            if (IsMine(pcoin->vout[i]))
            {
                CTxDestination address;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            };
    };

    set< set<CTxDestination>* > uniqueGroupings; // a set of pointers to groups of addresses
    map< CTxDestination, set<CTxDestination>* > setmap;  // map addresses to the unique group containing it
    BOOST_FOREACH(set<CTxDestination> grouping, groupings)
    {
        // make a set of all the groups hit by this new group
        set< set<CTxDestination>* > hits;
        map< CTxDestination, set<CTxDestination>* >::iterator it;
        BOOST_FOREACH(CTxDestination address, grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        // merge all hit groups into a new single group and delete old groups
        set<CTxDestination>* merged = new set<CTxDestination>(grouping);
        BOOST_FOREACH(set<CTxDestination>* hit, hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        };
        uniqueGroupings.insert(merged);

        // update setmap
        BOOST_FOREACH(CTxDestination element, *merged)
            setmap[element] = merged;
    };

    set< set<CTxDestination> > ret;
    BOOST_FOREACH(set<CTxDestination>* uniqueGrouping, uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    };

    return ret;
}

// ppcoin: check 'spent' consistency between wallet and txindex
// ppcoin: fix wallet spent state according to txindex
void CWallet::FixSpentCoins(int& nMismatchFound, int64_t& nBalanceInQuestion, bool fCheckOnly)
{
    if (nNodeMode != NT_FULL)
    {
        LogPrintf("FixSpentCoins must be run in full mode.\n");
        return;
    };

    nMismatchFound = 0;
    nBalanceInQuestion = 0;

    LOCK(cs_wallet);
    std::vector<CWalletTx*> vCoins;
    vCoins.reserve(mapWallet.size());
    for (WalletTxMap::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        vCoins.push_back(&(*it).second);

    CTxDB txdb("r");
    BOOST_FOREACH(CWalletTx* pcoin, vCoins)
    {
        // Find the corresponding transaction index
        CTxIndex txindex;
        if (!txdb.ReadTxIndex(pcoin->GetHash(), txindex))
            continue;
        for (unsigned int n=0; n < pcoin->vout.size(); n++)
        {
            if (IsMine(pcoin->vout[n]) && pcoin->IsSpent(n) && (txindex.vSpent.size() <= n || txindex.vSpent[n].IsNull()))
            {
                LogPrintf("FixSpentCoins found lost coin %s ALIAS (public) %s[%d], %s\n",
                    FormatMoney(pcoin->vout[n].nValue).c_str(), pcoin->GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                nMismatchFound++;
                nBalanceInQuestion += pcoin->vout[n].nValue;
                if (!fCheckOnly)
                {
                    pcoin->MarkUnspent(n);
                    pcoin->WriteToDisk();
                };
            } else
            if (IsMine(pcoin->vout[n]) && !pcoin->IsSpent(n) && (txindex.vSpent.size() > n && !txindex.vSpent[n].IsNull()))
            {
                LogPrintf("FixSpentCoins found spent coin %s ALIAS (public) %s[%d], %s\n",
                    FormatMoney(pcoin->vout[n].nValue).c_str(), pcoin->GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                nMismatchFound++;
                nBalanceInQuestion += pcoin->vout[n].nValue;
                if (!fCheckOnly)
                {
                    pcoin->MarkSpent(n);
                    pcoin->WriteToDisk();
                };
            };
        };
    };
}

// ppcoin: disable transaction (only for coinstake)
void CWallet::DisableTransaction(const CTransaction &tx)
{
    if (!tx.IsCoinStake() || !IsFromMe(tx))
        return; // only disconnecting coinstake requires marking input unspent

    LOCK(cs_wallet);
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (tx.nVersion == ANON_TXN_VERSION
            && txin.IsAnonInput())
            continue;
        WalletTxMap::iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size() && IsMine(prev.vout[txin.prevout.n]))
            {
                prev.MarkUnspent(txin.prevout.n);
                prev.WriteToDisk();
            };
        };
    };
}





int CWallet::GetChangeAddress(CPubKey &pk)
{
    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idDefaultAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    // - Return a key from the lookahead of the internal chain
    //   Don't promote the key to the main map, that will happen when the transaction is processed.

    CStoredExtKey *pc;
    if ((pc = mi->second->ChainInternal()) == NULL)
        return errorN(1, "%s Unknown chain.", __func__);

    uint32_t nChild;
    if (0 != pc->DeriveNextKey(pk, nChild, false, false))
        return errorN(1, "%s TryDeriveNext failed.", __func__);

    if (fDebug)
    {
        CBitcoinAddress addr(pk.GetID());
        LogPrintf("Change Address: %s\n", addr.ToString().c_str());
    };

    return 0;
};

int CWallet::ExtKeyNew32(CExtKey &out)
{
    if (fDebug)
        LogPrintf("ExtKeyNew32 from random.\n");

    uint8_t data[32];

    RandAddSeedPerfmon();
    for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
    {
        if (1 != RAND_bytes(data, 32))
            return errorN(1, "%s RAND_bytes failed.", __func__);

        if (ExtKeyNew32(out, data, 32) == 0)
            break;
    };

    return out.IsValid() ? 0 : 1;
};

int CWallet::ExtKeyNew32(CExtKey &out, const char *sPassPhrase, int32_t nHash, const char *sSeed)
{
    if (fDebug)
        LogPrintf("ExtKeyNew32 from pass.\n");

    uint8_t data[64];
    uint8_t passData[64];

    bool fPass = true;
    int nPhraseLen = strlen(sPassPhrase);
    int nSeedLen = strlen(sSeed);

    memset(passData, 0, 64);

    // - make the same key as http://bip32.org/
    HMAC_SHA256_CTX ctx256;
    for (int32_t i = 0; i < nHash; ++i)
    {
        HMAC_SHA256_Init(&ctx256, sPassPhrase, nPhraseLen);
        HMAC_SHA256_Update(&ctx256, passData, 32);
        HMAC_SHA256_Final(passData, &ctx256);
    };

    HMAC_SHA512_CTX ctx;

    if (!HMAC_SHA512_Init(&ctx, passData, 32))
        return errorN(1, "HMAC_SHA512_Init failed.");

    if (!HMAC_SHA512_Update(&ctx, sSeed, nSeedLen))
    {
        LogPrintf("HMAC_SHA512_Update failed.\n");
        fPass = false;
    };

    if (!HMAC_SHA512_Final(data, &ctx))
    {
        LogPrintf("HMAC_SHA512_Final failed.\n");
        fPass = false;
    };

    if (fPass && out.SetKeyCode(data, &data[32]) != 0)
        return errorN(1, "SetKeyCode failed.");

    return out.IsValid() ? 0 : 1;
};

int CWallet::ExtKeyNew32(CExtKey &out, uint8_t *data, uint32_t lenData)
{
    if (fDebug)
        LogPrintf("ExtKeyNew32.\n");

    out.SetMaster(data, lenData);

    return out.IsValid() ? 0 : 1;
};

int CWallet::ExtKeyImportLoose(CWalletDB *pwdb, CStoredExtKey &sekIn, bool fBip44, bool fSaveBip44)
{
    if (fDebug)
    {
        LogPrintf("ExtKeyImportLoose.\n");
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CKeyID id = sekIn.GetID();

    bool fsekInExist = true;
    // - it's possible for a public ext key to be added first
    CStoredExtKey sekExist;
    CStoredExtKey sek = sekIn;
    if (pwdb->ReadExtKey(id, sekExist))
    {
        if (IsCrypted()
            && 0 != ExtKeyUnlock(&sekExist))
            return errorN(13, "%s: %s", __func__, ExtKeyGetString(13));

        sek = sekExist;
        if (!sek.kp.IsValidV()
            && sekIn.kp.IsValidV())
        {
            sek.kp = sekIn.kp;
            std::vector<uint8_t> v;
            sek.mapValue[EKVT_ADDED_SECRET_AT] = SetCompressedInt64(v, GetTime());
        };
    } else
    {
        // - new key
        sek.nFlags |= EAF_ACTIVE;

        fsekInExist = false;
    };

    if (fBip44)
    {
        // import key as bip44 root and derive a new master key
        // NOTE: can't know created at time of derived key here

        std::vector<uint8_t> v;
        sek.mapValue[EKVT_KEY_TYPE] = SetChar(v, EKT_BIP44_MASTER);

        CExtKey evDerivedKey;
        sek.kp.Derive(evDerivedKey, BIP44_PURPOSE);
        evDerivedKey.Derive(evDerivedKey, Params().BIP44ID());

        v.resize(0);
        PushUInt32(v, BIP44_PURPOSE);
        PushUInt32(v, Params().BIP44ID());

        CStoredExtKey sekDerived;
        sekDerived.nFlags |= EAF_ACTIVE;
        sekDerived.kp = evDerivedKey;
        sekDerived.mapValue[EKVT_PATH] = v;
        sekDerived.mapValue[EKVT_ROOT_ID] = SetCKeyID(v, id);
        sekDerived.sLabel = sek.sLabel + " - bip44 derived.";

        CKeyID idDerived = sekDerived.GetID();

        if (pwdb->ReadExtKey(idDerived, sekExist))
        {
            if (fSaveBip44
                && !fsekInExist)
            {
                // - assume the user wants to save the bip44 key, drop down
            } else
            {
                return errorN(12, "%s: %s", __func__, ExtKeyGetString(12));
            };
        } else
        {
            if (IsCrypted()
                && (ExtKeyEncrypt(&sekDerived, vMasterKey, false) != 0))
                return errorN(1, "%s: ExtKeyEncrypt failed.", __func__);

            if (!pwdb->WriteExtKey(idDerived, sekDerived))
                return errorN(1, "%s: DB Write failed.", __func__);
        };
    };

    if (!fBip44 || fSaveBip44)
    {
        if (IsCrypted()
            && ExtKeyEncrypt(&sek, vMasterKey, false) != 0)
            return errorN(1, "%s: ExtKeyEncrypt failed.", __func__);

        if (!pwdb->WriteExtKey(id, sek))
            return errorN(1, "%s: DB Write failed.", __func__);
    };

    return 0;
};

int CWallet::ExtKeyImportAccount(CWalletDB *pwdb, CStoredExtKey &sekIn, int64_t nTimeStartScan, const std::string &sLabel)
{
    // rv: 0 success, 1 fail, 2 existing key, 3 updated key
    // It's not possible to import an account using only a public key as internal keys are derived hardened

    if (fDebug)
    {
        LogPrintf("ExtKeyImportAccount.\n");
        AssertLockHeld(cs_wallet);

        if (nTimeStartScan == 0)
            LogPrintf("No blockchain scanning.\n");
        else
            LogPrintf("Scan blockchain from %d.\n", nTimeStartScan);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CKeyID idAccount = sekIn.GetID();

    CStoredExtKey *sek = new CStoredExtKey();
    *sek = sekIn;

    // NOTE: is this confusing behaviour?
    CStoredExtKey sekExist;
    if (pwdb->ReadExtKey(idAccount, sekExist))
    {
        // add secret if exists in db
        *sek = sekExist;
        if (!sek->kp.IsValidV()
            && sekIn.kp.IsValidV())
        {
            sek->kp = sekIn.kp;
            std::vector<uint8_t> v;
            sek->mapValue[EKVT_ADDED_SECRET_AT] = SetCompressedInt64(v, GetTime());
        };
    };

    // TODO: before allowing import of 'watch only' accounts
    //       txns must be linked to accounts.

    if (!sek->kp.IsValidV())
    {
        delete sek;
        return errorN(1, "Accounts must be derived from a valid private key.");
    };

    CExtKeyAccount *sea = new CExtKeyAccount();
    if (pwdb->ReadExtAccount(idAccount, *sea))
    {
        if (0 != ExtKeyUnlock(sea))
        {
            delete sek;
            delete sea;
            return errorN(1, "Error unlocking existing account.");
        };
        CStoredExtKey *sekAccount = sea->ChainAccount();
        if (!sekAccount)
        {
            delete sek;
            delete sea;
            return errorN(1, "ChainAccount failed.");
        };
        // - account exists, update secret if necessary
        if (!sek->kp.IsValidV()
            && sekAccount->kp.IsValidV())
        {
            sekAccount->kp = sek->kp;
            std::vector<uint8_t> v;
            sekAccount->mapValue[EKVT_ADDED_SECRET_AT] = SetCompressedInt64(v, GetTime());

             if (IsCrypted()
                && ExtKeyEncrypt(sekAccount, vMasterKey, false) != 0)
            {
                delete sek;
                delete sea;
                return errorN(1, "ExtKeyEncrypt failed.");
            };

            if (!pwdb->WriteExtKey(idAccount, *sekAccount))
            {
                delete sek;
                delete sea;
                return errorN(1, "WriteExtKey failed.");
            };
            if (nTimeStartScan)
                ScanChainFromTime(nTimeStartScan);

            delete sek;
            delete sea;
            return 3;
        };
        delete sek;
        delete sea;
        return 2;
    };

    CKeyID idMaster(0);
    if (0 != ExtKeyCreateAccount(sek, idMaster, *sea, sLabel))
    {
        delete sek;
        delete sea;
        return errorN(1, "ExtKeyCreateAccount failed.");
    };

    std::vector<uint8_t> v;
    sea->mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, nTimeStartScan);

    if (0 != ExtKeySaveAccountToDB(pwdb, idAccount, sea))
    {
        sea->FreeChains();
        delete sea;
        return errorN(1, "DB Write failed.");
    };

    if (0 != ExtKeyAddAccountToMaps(idAccount, sea))
    {
        sea->FreeChains();
        delete sea;
        return errorN(1, "ExtKeyAddAccountToMap() failed.");
    };

    if (nTimeStartScan)
        ScanChainFromTime(nTimeStartScan);

    return 0;
};

int CWallet::ExtKeySetMaster(CWalletDB *pwdb, CKeyID &idNewMaster)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idNewMaster, CChainParams::EXT_KEY_HASH);
        LogPrintf("ExtKeySetMaster %s.\n", addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CKeyID idOldMaster;
    bool fOldMaster = pwdb->ReadNamedExtKeyId("master", idOldMaster);

    if (idNewMaster == idOldMaster)
        return errorN(11, ExtKeyGetString(11));

    ExtKeyMap::iterator mi;
    CStoredExtKey ekOldMaster, *pEkOldMaster, *pEkNewMaster;
    bool fNew = false;
    mi = mapExtKeys.find(idNewMaster);
    if (mi != mapExtKeys.end())
    {
        pEkNewMaster = mi->second;
    } else
    {
        pEkNewMaster = new CStoredExtKey();
        fNew = true;
        if (!pwdb->ReadExtKey(idNewMaster, *pEkNewMaster))
        {
            delete pEkNewMaster;
            return errorN(10, ExtKeyGetString(10));
        };
    };

    // - prevent setting bip44 root key as a master key.
    mapEKValue_t::iterator mvi = pEkNewMaster->mapValue.find(EKVT_KEY_TYPE);
    if (mvi != pEkNewMaster->mapValue.end()
        && mvi->second.size() == 1
        && mvi->second[0] == EKT_BIP44_MASTER)
    {
        if (fNew) delete pEkNewMaster;
        return errorN(9, ExtKeyGetString(9));
    };

    if (ExtKeyUnlock(pEkNewMaster) != 0
        || !pEkNewMaster->kp.IsValidV())
    {
        if (fNew) delete pEkNewMaster;
        return errorN(1, "New master ext key has no secret.");
    };

    std::vector<uint8_t> v;
    pEkNewMaster->mapValue[EKVT_KEY_TYPE] = SetChar(v, EKT_MASTER);

    if (!pwdb->WriteExtKey(idNewMaster, *pEkNewMaster)
        || !pwdb->WriteNamedExtKeyId("master", idNewMaster))
    {
        if (fNew) delete pEkNewMaster;
        return errorN(1, "DB Write failed.");
    };

    // -- unset old master ext key
    if (fOldMaster)
    {
        mi = mapExtKeys.find(idOldMaster);
        if (mi != mapExtKeys.end())
        {
            pEkOldMaster = mi->second;
        } else
        {
            if (!pwdb->ReadExtKey(idOldMaster, ekOldMaster))
            {
                if (fNew) delete pEkNewMaster;
                return errorN(1, "ReadExtKey failed.");
            };

            pEkOldMaster = &ekOldMaster;
        };

        mapEKValue_t::iterator it = pEkOldMaster->mapValue.find(EKVT_KEY_TYPE);
        if (it != pEkOldMaster->mapValue.end())
        {
            if (fDebug)
                LogPrintf("Removing tag from old master key %s.\n", pEkOldMaster->GetIDString58().c_str());
            pEkOldMaster->mapValue.erase(it);
            if (!pwdb->WriteExtKey(idOldMaster, *pEkOldMaster))
            {
                if (fNew) delete pEkNewMaster;
                return errorN(1, "WriteExtKey failed.");
            };
        };
    };

    mapExtKeys[idNewMaster] = pEkNewMaster;
    pEkMaster = pEkNewMaster;

    return 0;
};

int CWallet::ExtKeyNewMaster(CWalletDB *pwdb, CKeyID &idMaster, bool fAutoGenerated, CExtKey* pRootKey)
{
    // - Must pair with ExtKeySetMaster

    //  This creates two keys, a root key and a master key derived according
    //  to BIP44 (path 44'/22'), The root (bip44) key only stored in the system
    //  and the derived key is set as the system master key.

    LogPrintf("ExtKeyNewMaster.\n");
    AssertLockHeld(cs_wallet);
    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CExtKey evRootKey;
    CStoredExtKey sekRoot;

    if (pRootKey)
        evRootKey = *pRootKey;
    else if (ExtKeyNew32(evRootKey) != 0)
        return errorN(1, "ExtKeyNew32 failed.");

    std::vector<uint8_t> v;
    sekRoot.nFlags |= EAF_ACTIVE;
    sekRoot.mapValue[EKVT_KEY_TYPE] = SetChar(v, EKT_BIP44_MASTER);
    sekRoot.kp = evRootKey;
    sekRoot.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sekRoot.sLabel = "Initial BIP44 Master";
    CKeyID idRoot = sekRoot.GetID();

    CExtKey evMasterKey;
    evRootKey.Derive(evMasterKey, BIP44_PURPOSE);
    evMasterKey.Derive(evMasterKey, Params().BIP44ID());

    std::vector<uint8_t> vPath;
    PushUInt32(vPath, BIP44_PURPOSE);
    PushUInt32(vPath, Params().BIP44ID());

    CStoredExtKey sekMaster;
    sekMaster.nFlags |= EAF_ACTIVE;
    sekMaster.kp = evMasterKey;
    sekMaster.mapValue[EKVT_PATH] = vPath;
    sekMaster.mapValue[EKVT_ROOT_ID] = SetCKeyID(v, idRoot);
    sekMaster.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sekMaster.sLabel = "Initial Master";

    idMaster = sekMaster.GetID();

    if (IsCrypted()
        && (ExtKeyEncrypt(&sekRoot, vMasterKey, false) != 0
            || ExtKeyEncrypt(&sekMaster, vMasterKey, false) != 0))
    {
        return errorN(1, "ExtKeyEncrypt failed.");
    };

    if (!pwdb->WriteExtKey(idRoot, sekRoot)
        || !pwdb->WriteExtKey(idMaster, sekMaster)
        || (fAutoGenerated && !pwdb->WriteFlag("madeDefaultEKey", 1)))
    {
        return errorN(1, "DB Write failed.");
    };

    return 0;
};


int CWallet::ExtKeyCreateAccount(CStoredExtKey *sekAccount, CKeyID &idMaster, CExtKeyAccount &ekaOut, const std::string &sLabel)
{
    if (fDebug)
    {
        LogPrintf("ExtKeyCreateAccount.\n");
        AssertLockHeld(cs_wallet);
    };

    std::vector<uint8_t> vAccountPath, vSubKeyPath, v;
    mapEKValue_t::iterator mi = sekAccount->mapValue.find(EKVT_PATH);

    if (mi != sekAccount->mapValue.end())
    {
        vAccountPath = mi->second;
    };

    ekaOut.idMaster = idMaster;
    ekaOut.sLabel = sLabel;
    ekaOut.nFlags |= EAF_ACTIVE;
    ekaOut.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());

    if (sekAccount->kp.IsValidV())
        ekaOut.nFlags |= EAF_HAVE_SECRET;

    CExtKey evExternal, evInternal, evStealth;
    uint32_t nExternal = 0, nInternal = 0, nStealth = 0;
    if (sekAccount->DeriveNextKey(evExternal, nExternal, false) != 0
        || sekAccount->DeriveNextKey(evInternal, nInternal, false) != 0
        || sekAccount->DeriveNextKey(evStealth, nStealth, true) != 0)
    {
        return errorN(1, "Could not derive account chain keys.");
    };

    CStoredExtKey *sekExternal = new CStoredExtKey();
    sekExternal->kp = evExternal;
    vSubKeyPath = vAccountPath;
    sekExternal->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nExternal);
    sekExternal->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT;
    sekExternal->mapValue[EKVT_N_LOOKAHEAD] = SetCompressedInt64(v, N_DEFAULT_EKVT_LOOKAHEAD);

    CStoredExtKey *sekInternal = new CStoredExtKey();
    sekInternal->kp = evInternal;
    vSubKeyPath = vAccountPath;
    sekInternal->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nInternal);
    sekInternal->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT;

    CStoredExtKey *sekStealth = new CStoredExtKey();
    sekStealth->kp = evStealth;
    vSubKeyPath = vAccountPath;
    sekStealth->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nStealth);
    sekStealth->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT;

    ekaOut.vExtKeyIDs.push_back(sekAccount->GetID());
    ekaOut.vExtKeyIDs.push_back(sekExternal->GetID());
    ekaOut.vExtKeyIDs.push_back(sekInternal->GetID());
    ekaOut.vExtKeyIDs.push_back(sekStealth->GetID());

    ekaOut.vExtKeys.push_back(sekAccount);
    ekaOut.vExtKeys.push_back(sekExternal);
    ekaOut.vExtKeys.push_back(sekInternal);
    ekaOut.vExtKeys.push_back(sekStealth);

    ekaOut.nActiveExternal = 1;
    ekaOut.nActiveInternal = 2;
    ekaOut.nActiveStealth = 3;

    if (IsCrypted()
        && ExtKeyEncrypt(&ekaOut, vMasterKey, false) != 0)
    {
        delete sekExternal;
        delete sekInternal;
        delete sekStealth;
        // - sekAccount should be freed in calling function
        return errorN(1, "ExtKeyEncrypt failed.");
    };

    return 0;
};

int CWallet::ExtKeyDeriveNewAccount(CWalletDB *pwdb, CExtKeyAccount *sea, const std::string &sLabel, const std::string &sPath)
{
    // - derive a new account from the master extkey and save to wallet
    LogPrintf("%s\n", __func__);
    AssertLockHeld(cs_wallet);
    assert(pwdb);
    assert(sea);

    if (IsLocked())
        return errorN(1, "%s: Wallet must be unlocked.", __func__);

    if (!pEkMaster || !pEkMaster->kp.IsValidV())
        return errorN(1, "%s: Master ext key is invalid.", __func__);

    CKeyID idMaster = pEkMaster->GetID();

    CStoredExtKey *sekAccount = new CStoredExtKey();
    CExtKey evAccountKey;
    uint32_t nOldHGen = pEkMaster->GetCounter(true);
    uint32_t nAccount;
    std::vector<uint8_t> vAccountPath, vSubKeyPath;

    if (sPath.length() == 0)
    {
        if (pEkMaster->DeriveNextKey(evAccountKey, nAccount, true) != 0)
        {
            delete sekAccount;
            return errorN(1, "%s: Could not derive account key from master.", __func__);
        };
        sekAccount->kp = evAccountKey;
        sekAccount->mapValue[EKVT_PATH] = PushUInt32(vAccountPath, nAccount);
    } else
    {
        std::vector<uint32_t> vPath;
        int rv;
        if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
        {
            delete sekAccount;
            return errorN(1, "%s: ExtractExtKeyPath failed %s.", __func__, ExtKeyGetString(rv));
        };

        CExtKey vkOut;
        CExtKey vkWork = pEkMaster->kp.GetExtKey();
        for (std::vector<uint32_t>::iterator it = vPath.begin(); it != vPath.end(); ++it)
        {

            if (!vkWork.Derive(vkOut, *it))
            {
                delete sekAccount;
                return errorN(1, "%s: CExtKey Derive failed %s, %d.", __func__, sPath.c_str(), *it);
            };
            PushUInt32(vAccountPath, *it);

            vkWork = vkOut;
        };

        sekAccount->kp = vkOut;
        sekAccount->mapValue[EKVT_PATH] = vAccountPath;
    };

    if (!sekAccount->kp.IsValidV()
        || !sekAccount->kp.IsValidP())
    {
        delete sekAccount;
        pEkMaster->SetCounter(nOldHGen, true);
        return errorN(1, "%s: Invalid key.", __func__);
    };

    sekAccount->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT;
    if (0 != ExtKeyCreateAccount(sekAccount, idMaster, *sea, sLabel))
    {
        delete sekAccount;
        pEkMaster->SetCounter(nOldHGen, true);
        return errorN(1, "%s: ExtKeyCreateAccount failed.", __func__);
    };

    CKeyID idAccount = sea->GetID();

    if (!pwdb->WriteExtKey(idMaster, *pEkMaster)
        || 0 != ExtKeySaveAccountToDB(pwdb, idAccount, sea))
    {
        sea->FreeChains();
        pEkMaster->SetCounter(nOldHGen, true);
        return errorN(1, "%s: DB Write failed.", __func__);
    };

    if (0 != ExtKeyAddAccountToMaps(idAccount, sea))
    {
        sea->FreeChains();
        return errorN(1, "%s: ExtKeyAddAccountToMaps() failed.", __func__);
    };

    return 0;
};


int CWallet::ExtKeyEncrypt(CStoredExtKey *sek, const CKeyingMaterial &vMKey, bool fLockKey)
{
    if (!sek->kp.IsValidV())
    {
        if (fDebug)
            LogPrintf("%s: sek %s has no secret, encryption skipped.", __func__, sek->GetIDString58());
        return 0;
        //return errorN(1, "Invalid secret.");
    };

    std::vector<uint8_t> vchCryptedSecret;
    CPubKey pubkey = sek->kp.pubkey;
    CKeyingMaterial vchSecret(sek->kp.key.begin(), sek->kp.key.end());
    if (!EncryptSecret(vMKey, vchSecret, pubkey.GetHash(), vchCryptedSecret))
        return errorN(1, "EncryptSecret failed.");

    sek->nFlags |= EAF_IS_CRYPTED;

    sek->vchCryptedSecret = vchCryptedSecret;

    // - CStoredExtKey serialise will never save key when vchCryptedSecret is set
    //   thus key can be left intact here
    if (fLockKey)
    {
        sek->fLocked = 1;
        sek->kp.key.Clear();
    } else
    {
        sek->fLocked = 0;
    };

    return 0;
};

int CWallet::ExtKeyEncrypt(CExtKeyAccount *sea, const CKeyingMaterial &vMKey, bool fLockKey)
{
    assert(sea);

    std::vector<CStoredExtKey*>::iterator it;
    for (it = sea->vExtKeys.begin(); it != sea->vExtKeys.end(); ++it)
    {
        CStoredExtKey *sek = *it;
        if (sek->nFlags & EAF_IS_CRYPTED)
            continue;

        if (!sek->kp.IsValidV()
            && fDebug)
        {
            LogPrintf("%s : Skipping account %s chain, no secret.", __func__, sea->GetIDString58().c_str());
            continue;
        };

        if (sek->kp.IsValidV()
            && ExtKeyEncrypt(sek, vMKey, fLockKey) != 0)
            return 1;
    };

    return 0;
};

int CWallet::ExtKeyEncryptAll(CWalletDB *pwdb, const CKeyingMaterial &vMKey)
{
    LogPrintf("%s\n", __func__);

    // Encrypt loose and account extkeys stored in wallet
    // skip invalid private keys

    Dbc *pcursor = pwdb->GetTxnCursor();

    if (!pcursor)
        return errorN(1, "%s : cannot create DB cursor.", __func__);

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CKeyID ckeyId;
    CBitcoinAddress addr;
    CStoredExtKey sek;
    CExtKeyAccount sea;
    CExtKey58 eKey58;
    std::string strType;

    size_t nKeys = 0;
    size_t nAccounts = 0;

    uint32_t fFlags = DB_SET_RANGE;
    ssKey << std::string("ek32");
    while (pwdb->ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "ek32")
            break;

        ssKey >> ckeyId;
        ssValue >> sek;

        if (!sek.kp.IsValidV())
        {
            if (fDebug)
            {
                addr.Set(ckeyId, CChainParams::EXT_KEY_HASH);
                LogPrintf("%s : Skipping key %s, no secret.", __func__, sek.GetIDString58().c_str());
            };
            continue;
        };

        if (ExtKeyEncrypt(&sek, vMKey, true) != 0)
            return errorN(1, "%s : ExtKeyEncrypt failed.", __func__);

        nKeys++;

        if (!pwdb->Replace(pcursor, sek))
            return errorN(1, "%s : Replace failed.", __func__);
    };

    pcursor->close();

    if (fDebug)
        LogPrintf("%s : Encrypted %u keys, %u accounts.", __func__, nKeys, nAccounts);

    return 0;
};

int CWallet::ExtKeyLock()
{
    if (fDebug)
        LogPrintf("ExtKeyLock.\n");

    if (pEkMaster)
    {
        pEkMaster->kp.key.Clear();
        pEkMaster->fLocked = 1;
    };

    // TODO: iterate over mapExtKeys instead?
    ExtKeyAccountMap::iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *sea = mi->second;
        std::vector<CStoredExtKey*>::iterator it;
        for (it = sea->vExtKeys.begin(); it != sea->vExtKeys.end(); ++it)
        {
            CStoredExtKey *sek = *it;
            if (!(sek->nFlags & EAF_IS_CRYPTED))
                continue;
            sek->kp.key.Clear();
            sek->fLocked = 1;
        };
    };

    return 0;
};




int CWallet::ExtKeyUnlock(CExtKeyAccount *sea)
{
    return ExtKeyUnlock(sea, vMasterKey);
};

int CWallet::ExtKeyUnlock(CExtKeyAccount *sea, const CKeyingMaterial &vMKey)
{
    std::vector<CStoredExtKey*>::iterator it;
    for (it = sea->vExtKeys.begin(); it != sea->vExtKeys.end(); ++it)
    {
        CStoredExtKey *sek = *it;
        if (!(sek->nFlags & EAF_IS_CRYPTED))
            continue;
        if (ExtKeyUnlock(sek, vMKey) != 0)
            return 1;
    };

    return 0;
};

int CWallet::ExtKeyUnlock(CStoredExtKey *sek)
{
    return ExtKeyUnlock(sek, vMasterKey);
};

int CWallet::ExtKeyUnlock(CStoredExtKey *sek, const CKeyingMaterial &vMKey)
{
    if (!(sek->nFlags & EAF_IS_CRYPTED)) // is not necessary to unlock
        return 0;

    CSecret vchSecret;
    uint256 iv = Hash(sek->kp.pubkey.begin(), sek->kp.pubkey.end());
    if (!DecryptSecret(vMKey, sek->vchCryptedSecret, iv, vchSecret)
        || vchSecret.size() != 32)
    {
        return errorN(1, "Failed decrypting ext key %s", sek->GetIDString58().c_str());
    };

    sek->kp.key.Set(vchSecret.begin(), vchSecret.end(), true);

    if (!sek->kp.IsValidV())
        return errorN(1, "Failed decrypting ext key %s", sek->GetIDString58().c_str());

    // - check, necessary?
    if (sek->kp.key.GetPubKey() != sek->kp.pubkey)
        return errorN(1, "Decrypted ext key mismatch %s", sek->GetIDString58().c_str());

    sek->fLocked = 0;
    return 0;
};

int CWallet::ExtKeyUnlock(const CKeyingMaterial &vMKey)
{
    if (fDebug)
        LogPrintf("ExtKeyUnlock.\n");

    if (pEkMaster
        && pEkMaster->nFlags & EAF_IS_CRYPTED)
    {
        if (ExtKeyUnlock(pEkMaster, vMKey) != 0)
            return 1;
    };

    ExtKeyAccountMap::iterator mi;
    mi = mapExtAccounts.begin();
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *sea = mi->second;

        if (ExtKeyUnlock(sea, vMKey) != 0)
            return errorN(1, "ExtKeyUnlock() account failed.");
    };

    return 0;
};


int CWallet::ExtKeyCreateInitial(CWalletDB *pwdb, std::string sBip44Key)
{
    LogPrintf("Creating intital extended master key and account.\n");

    CKeyID idMaster;

    if (!pwdb->TxnBegin())
        return errorN(1, "TxnBegin failed.");

    CExtKey ekBip44;
    if (!sBip44Key.empty())
    {
        CExtKey58 eKey58;
        if (eKey58.Set58(sBip44Key.c_str()) == 0)
        {
            if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY_BTC))
            {
                pwdb->TxnAbort();
                return errorN(1, "-bip44key defines invalid key. Key must begin with Alias prefix.");
            }
            ekBip44 = eKey58.GetKey().GetExtKey();
            LogPrintf("Using given -bip44key for initial master key.\n");
        } else
        {
            pwdb->TxnAbort();
            return errorN(1, "-bip44key defines invalid key");
        };
    }

    if (ExtKeyNewMaster(pwdb, idMaster, true, ekBip44.IsValid() ? &ekBip44 : nullptr ) != 0
        || ExtKeySetMaster(pwdb, idMaster) != 0)
    {
        pwdb->TxnAbort();
        return errorN(1, "Make or SetNewMasterKey failed.");
    };

    CExtKeyAccount *seaDefault = new CExtKeyAccount();

    if (ExtKeyDeriveNewAccount(pwdb, seaDefault, "default") != 0)
    {
        delete seaDefault;
        pwdb->TxnAbort();
        return errorN(1, "DeriveNewExtAccount failed.");
    };


    idDefaultAccount = seaDefault->GetID();
    if (!pwdb->WriteNamedExtKeyId("defaultAccount", idDefaultAccount))
    {
        pwdb->TxnAbort();
        return errorN(1, "WriteNamedExtKeyId failed.");
    };

    CPubKey newKey;
    if (0 != NewKeyFromAccount(pwdb, idDefaultAccount, newKey, false, false))
    {
        pwdb->TxnAbort();
        return errorN(1, "NewKeyFromAccount failed.");
    }

    CEKAStealthKey aks;
    string strLbl = "Default Private Address";
    if (0 != NewStealthKeyFromAccount(pwdb, idDefaultAccount, strLbl, aks))
    {
        pwdb->TxnAbort();
        return errorN(1, "NewKeyFromAccount failed.");
    }

    if (!pwdb->TxnCommit())
    {
        // --TxnCommit destroys activeTxn
        return errorN(1, "TxnCommit failed.");
    };

    SetAddressBookName(CBitcoinAddress(newKey.GetID()).Get(), "Default Public Address", NULL, true, true);

    return 0;
}

int CWallet::ExtKeyLoadMaster()
{
    LogPrintf("Loading master ext key.\n");

    LOCK(cs_wallet);

    CKeyID idMaster;

    CWalletDB wdb(strWalletFile, "r+");
    if (!wdb.ReadNamedExtKeyId("master", idMaster))
    {
        int nValue;
        if (!wdb.ReadFlag("madeDefaultEKey", nValue)
            || nValue == 0)
        {
            if (IsLocked())
            {
                fMakeExtKeyInitials = true; // set flag for unlock
                LogPrintf("Wallet locked, master key will be created when unlocked.\n");
                return 0;
            };

            if (ExtKeyCreateInitial(&wdb, GetArg("-bip44key", "")) != 0)
                return errorN(1, "ExtKeyCreateDefaultMaster failed.");

            return 0;
        };
        LogPrintf("Warning: No master ext key has been set.\n");
        return 1;
    };

    pEkMaster = new CStoredExtKey();
    if (!wdb.ReadExtKey(idMaster, *pEkMaster))
    {
        delete pEkMaster;
        pEkMaster = NULL;
        return errorN(1, "ReadExtKey failed to read master ext key.");
    };

    if (!pEkMaster->kp.IsValidP()) // wallet could be locked, check pk
    {
        delete pEkMaster;
        pEkMaster = NULL;
        return errorN(1, " Loaded master ext key is invalid %s.", pEkMaster->GetIDString58().c_str());
    };

    if (pEkMaster->nFlags & EAF_IS_CRYPTED)
        pEkMaster->fLocked = 1;

    // - add to key map
    mapExtKeys[idMaster] = pEkMaster;

    // find earliest key creation time, as wallet birthday
    int64_t nCreatedAt;
    GetCompressedInt64(pEkMaster->mapValue[EKVT_CREATED_AT], (uint64_t&)nCreatedAt);

    if (!nTimeFirstKey || (nCreatedAt && nCreatedAt < nTimeFirstKey))
        nTimeFirstKey = nCreatedAt;

    return 0;
};

int CWallet::ExtKeyLoadAccounts()
{
    LogPrintf("Loading ext accounts.\n");

    LOCK(cs_wallet);

    CWalletDB wdb(strWalletFile);

    if (!wdb.ReadNamedExtKeyId("defaultAccount", idDefaultAccount))
    {
        LogPrintf("Warning: No default ext account set.\n");
    };

    Dbc *pcursor;
    if (!(pcursor = wdb.GetAtCursor()))
        throw std::runtime_error(strprintf("%s: cannot create DB cursor", __func__).c_str());

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CBitcoinAddress addr;
    CKeyID idAccount;
    std::string strType;

    unsigned int fFlags = DB_SET_RANGE;
    ssKey << std::string("eacc");
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "eacc")
            break;

        ssKey >> idAccount;

        if (fDebug)
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Loading account %s\n", addr.ToString().c_str());
        };

        CExtKeyAccount *sea = new CExtKeyAccount();
        ssValue >> *sea;

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi != mapExtAccounts.end())
        {
            // - account already loaded, skip, can be caused by ExtKeyCreateInitial()
            if (fDebug)
                LogPrintf("Account already loaded.\n");
            continue;
        };

        if (!(sea->nFlags & EAF_ACTIVE))
        {
            if (fDebug)
            {
                addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
                LogPrintf("Skipping inactive %s\n", addr.ToString().c_str());
            };
            delete sea;
            continue;
        };

        // find earliest key creation time, as wallet birthday
        int64_t nCreatedAt;
        GetCompressedInt64(sea->mapValue[EKVT_CREATED_AT], (uint64_t&)nCreatedAt);

        if (!nTimeFirstKey || (nCreatedAt && nCreatedAt < nTimeFirstKey))
            nTimeFirstKey = nCreatedAt;

        sea->vExtKeys.resize(sea->vExtKeyIDs.size());
        for (size_t i = 0; i < sea->vExtKeyIDs.size(); ++i)
        {
            CKeyID &id = sea->vExtKeyIDs[i];
            CStoredExtKey *sek = new CStoredExtKey();

            if (wdb.ReadExtKey(id, *sek))
            {
                sea->vExtKeys[i] = sek;
            } else
            {
                addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
                LogPrintf("WARNING: Could not read key %d of account %s\n", i, addr.ToString().c_str());
                sea->vExtKeys[i] = NULL;
                delete sek;
            };
        };

        if (0 != ExtKeyAddAccountToMaps(idAccount, sea))
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("ExtKeyAddAccountToMaps() failed: %s\n", addr.ToString().c_str());
            sea->FreeChains();
            delete sea;
        };
    };


    pcursor->close();

    return 0;
};

int CWallet::ExtKeySaveAccountToDB(CWalletDB *pwdb, CKeyID &idAccount, CExtKeyAccount *sea)
{
    if (fDebug)
    {
        LogPrintf("ExtKeySaveAccountToDB()\n");
        AssertLockHeld(cs_wallet);
    };
    assert(sea);

    for (size_t i = 0; i < sea->vExtKeys.size(); ++i)
    {
        CStoredExtKey *sek = sea->vExtKeys[i];
        if (!pwdb->WriteExtKey(sea->vExtKeyIDs[i], *sek))
            return errorN(1, "ExtKeySaveAccountToDB(): WriteExtKey failed.");
    };

    if (!pwdb->WriteExtAccount(idAccount, *sea))
        return errorN(1, "ExtKeySaveAccountToDB() WriteExtAccount failed.");

    return 0;
};

int CWallet::ExtKeyAddAccountToMaps(CKeyID &idAccount, CExtKeyAccount *sea)
{
    // - open/activate account in wallet
    //   add to mapExtAccounts and mapExtKeys

    if (fDebug)
    {
        LogPrintf("ExtKeyAddAccountToMap()\n");
        AssertLockHeld(cs_wallet);
    };
    assert(sea);

    for (size_t i = 0; i < sea->vExtKeys.size(); ++i)
    {
        CStoredExtKey *sek = sea->vExtKeys[i];

        if (sek->nFlags & EAF_IS_CRYPTED)
            sek->fLocked = 1;

        if (sek->nFlags & EAF_ACTIVE
            && sek->nFlags & EAF_RECEIVE_ON)
        {
            uint64_t nLookAhead = N_DEFAULT_LOOKAHEAD;

            mapEKValue_t::iterator itV = sek->mapValue.find(EKVT_N_LOOKAHEAD);
            if (itV != sek->mapValue.end())
                nLookAhead = GetCompressedInt64(itV->second, nLookAhead);

            sea->AddLookAhead(i, (uint32_t)nLookAhead);
        };

        mapExtKeys[sea->vExtKeyIDs[i]] = sek;
    };

    mapExtAccounts[idAccount] = sea;
    return 0;
};

int CWallet::ExtKeyLoadAccountPacks()
{
    LogPrintf("Loading ext account packs.\n");

    LOCK(cs_wallet);

    CWalletDB wdb(strWalletFile);

    Dbc *pcursor;
    if (!(pcursor = wdb.GetAtCursor()))
        throw std::runtime_error(strprintf("%s : cannot create DB cursor", __func__).c_str());

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CKeyID idAccount;
    CBitcoinAddress addr;
    uint32_t nPack;
    std::string strType;
    std::vector<CEKAKeyPack> ekPak;
    std::vector<CEKAStealthKeyPack> aksPak;
    std::vector<CEKASCKeyPack> asckPak;

    unsigned int fFlags = DB_SET_RANGE;
    ssKey << std::string("epak");
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        ekPak.clear();
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "epak")
            break;

        ssKey >> idAccount;
        ssKey >> nPack;

        if (fDebug)
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Loading account key pack %s %u\n", addr.ToString().c_str(), nPack);
        };

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi == mapExtAccounts.end())
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Warning: Unknown account %s.\n", addr.ToString().c_str());
            continue;
        };

        CExtKeyAccount *sea = mi->second;

        ssValue >> ekPak;

        std::vector<CEKAKeyPack>::iterator it;
        for (it = ekPak.begin(); it != ekPak.end(); ++it)
        {
            sea->mapKeys[it->id] = it->ak;
        };
    };

    ssKey.clear();
    ssKey << std::string("espk");
    fFlags = DB_SET_RANGE;
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        aksPak.clear();
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "espk")
            break;

        ssKey >> idAccount;
        ssKey >> nPack;

        if (fDebug)
            LogPrintf("Loading account stealth key pack %s %u\n", idAccount.ToString().c_str(), nPack);

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi == mapExtAccounts.end())
        {
            CBitcoinAddress addr;
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Warning: Unknown account %s.\n", addr.ToString().c_str());
            continue;
        };

        CExtKeyAccount *sea = mi->second;

        ssValue >> aksPak;

        std::vector<CEKAStealthKeyPack>::iterator it;
        for (it = aksPak.begin(); it != aksPak.end(); ++it)
        {
            sea->mapStealthKeys[it->id] = it->aks;
        };
    };

    ssKey.clear();
    ssKey << std::string("ecpk");
    fFlags = DB_SET_RANGE;
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        aksPak.clear();
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "ecpk")
            break;

        ssKey >> idAccount;
        ssKey >> nPack;

        if (fDebug)
            LogPrintf("Loading account stealth child key pack %s %u\n", idAccount.ToString().c_str(), nPack);

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi == mapExtAccounts.end())
        {
            CBitcoinAddress addr;
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Warning: Unknown account %s.\n", addr.ToString().c_str());
            continue;
        };

        CExtKeyAccount *sea = mi->second;

        ssValue >> asckPak;

        std::vector<CEKASCKeyPack>::iterator it;
        for (it = asckPak.begin(); it != asckPak.end(); ++it)
        {
            sea->mapStealthChildKeys[it->id] = it->asck;
        };
    };

    pcursor->close();

    return 0;
};

int CWallet::ExtKeyAppendToPack(CWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &idKey, CEKAKey &ak, bool &fUpdateAcc) const
{
    // - must call WriteExtAccount after


    CKeyID idAccount = sea->GetID();
    std::vector<CEKAKeyPack> ekPak;
    if (!pwdb->ReadExtKeyPack(idAccount, sea->nPack, ekPak))
    {
        // -- new pack
        ekPak.clear();
        if (fDebug)
            LogPrintf("Account %s, starting new keypack %u.\n", idAccount.ToString(), sea->nPack);
    };

    try { ekPak.push_back(CEKAKeyPack(idKey, ak)); } catch (std::exception& e)
    {
        return errorN(1, "%s push_back failed.", __func__, sea->nPack);
    };

    if (!pwdb->WriteExtKeyPack(idAccount, sea->nPack, ekPak))
    {
        return errorN(1, "%s Save key pack %u failed.", __func__, sea->nPack);
    };

    fUpdateAcc = false;
    if ((uint32_t)ekPak.size() >= MAX_KEY_PACK_SIZE-1)
    {
        fUpdateAcc = true;
        sea->nPack++;
    };
    return 0;
};

int CWallet::ExtKeyAppendToPack(CWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &idKey, CEKASCKey &asck, bool &fUpdateAcc) const
{

    // - must call WriteExtAccount after

    CKeyID idAccount = sea->GetID();
    std::vector<CEKASCKeyPack> asckPak;
    if (!pwdb->ReadExtStealthKeyChildPack(idAccount, sea->nPackStealthKeys, asckPak))
    {
        // -- new pack
        asckPak.clear();
        if (fDebug)
            LogPrintf("Account %s, starting new stealth child keypack %u.\n", idAccount.ToString(), sea->nPackStealthKeys);
    };

    try { asckPak.push_back(CEKASCKeyPack(idKey, asck)); } catch (std::exception& e)
    {
        return errorN(1, "%s push_back failed.", __func__);
    };

    if (!pwdb->WriteExtStealthKeyChildPack(idAccount, sea->nPackStealthKeys, asckPak))
        return errorN(1, "%s Save key pack %u failed.", __func__, sea->nPackStealthKeys);

    fUpdateAcc = false;
    if ((uint32_t)asckPak.size() >= MAX_KEY_PACK_SIZE-1)
    {
        sea->nPackStealthKeys++;
        fUpdateAcc = true;
    };

    return 0;
};

int CWallet::ExtKeySaveKey(CWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &keyId, CEKAKey &ak) const
{
    if (fDebug)
    {
        CBitcoinAddress addr(keyId);
        LogPrintf("%s %s %s.\n", __func__, sea->GetIDString58().c_str(), addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    if (!sea->SaveKey(keyId, ak))
        return errorN(1, "%s SaveKey failed.", __func__);

    bool fUpdateAcc;
    if (0 != ExtKeyAppendToPack(pwdb, sea, keyId, ak, fUpdateAcc))
        return errorN(1, "%s ExtKeyAppendToPack failed.", __func__);

    CStoredExtKey *pc = sea->GetChain(ak.nParent);
    if (!pc)
        return errorN(1, "%s GetChain failed.", __func__);

    CKeyID idChain = sea->vExtKeyIDs[ak.nParent];
    if (!pwdb->WriteExtKey(idChain, *pc))
        return errorN(1, "%s WriteExtKey failed.", __func__);

    if (fUpdateAcc) // only neccessary if nPack has changed
    {
        CKeyID idAccount = sea->GetID();
        if (!pwdb->WriteExtAccount(idAccount, *sea))
            return errorN(1, "%s WriteExtAccount failed.", __func__);
    };

    return 0;
};

int CWallet::ExtKeySaveKey(CExtKeyAccount *sea, const CKeyID &keyId, CEKAKey &ak) const
{

    //LOCK(cs_wallet);
    if (fDebug)
        AssertLockHeld(cs_wallet);

    CWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != ExtKeySaveKey(&wdb, sea, keyId, ak))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);
    return 0;
};

int CWallet::ExtKeySaveKey(CWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &keyId, CEKASCKey &asck) const
{
    if (fDebug)
    {
        CBitcoinAddress addr(keyId);
        LogPrintf("%s %s %s.\n", __func__, sea->GetIDString58().c_str(), addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    if (!sea->SaveKey(keyId, asck))
        return errorN(1, "%s SaveKey failed.", __func__);

    bool fUpdateAcc;
    if (0 != ExtKeyAppendToPack(pwdb, sea, keyId, asck, fUpdateAcc))
        return errorN(1, "%s ExtKeyAppendToPack failed.", __func__);

    if (fUpdateAcc) // only neccessary if nPackStealth has changed
    {
        CKeyID idAccount = sea->GetID();
        if (!pwdb->WriteExtAccount(idAccount, *sea))
            return errorN(1, "%s WriteExtAccount failed.", __func__);
    };

    return 0;
};

int CWallet::ExtKeySaveKey(CExtKeyAccount *sea, const CKeyID &keyId, CEKASCKey &asck) const
{
    if (fDebug)
        AssertLockHeld(cs_wallet);

    CWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != ExtKeySaveKey(&wdb, sea, keyId, asck))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);
    return 0;
};

int CWallet::ExtKeyUpdateStealthAddress(CWalletDB *pwdb, CExtKeyAccount *sea, CKeyID &sxId, std::string &sLabel)
{
    if (fDebug)
    {
        LogPrintf("%s.\n", __func__);
        AssertLockHeld(cs_wallet);
    };

    AccStealthKeyMap::iterator it = sea->mapStealthKeys.find(sxId);
    if (it == sea->mapStealthKeys.end())
        return errorN(1, "%s: Stealth key not in account.", __func__);


    if (it->second.sLabel == sLabel)
        return 0; // no change

    CKeyID accId = sea->GetID();
    std::vector<CEKAStealthKeyPack> aksPak;
    for (uint32_t i = 0; i <= sea->nPackStealth; ++i)
    {
        if (!pwdb->ReadExtStealthKeyPack(accId, i, aksPak))
            return errorN(1, "%s: ReadExtStealthKeyPack %d failed.", __func__, i);

        std::vector<CEKAStealthKeyPack>::iterator itp;
        for (itp = aksPak.begin(); itp != aksPak.end(); ++itp)
        {
            if (itp->id == sxId)
            {
                itp->aks.sLabel = string(sLabel);
                if (!pwdb->WriteExtStealthKeyPack(accId, i, aksPak))
                    return errorN(1, "%s: WriteExtStealthKeyPack %d failed.", __func__, i);

                it->second.sLabel = string(sLabel);

                CStealthAddress cekaSxAddr;
                if (0 == itp->aks.SetSxAddr(cekaSxAddr))
                    NotifyAddressBookChanged(this, cekaSxAddr, cekaSxAddr.label, true, CT_UPDATED, true);

                return 0;
            };
        };
    };

    return errorN(1, "%s: Stealth key not in db.", __func__);
};

int CWallet::NewKeyFromAccount(CWalletDB *pwdb, const CKeyID &idAccount, CPubKey &pkOut, bool fInternal, bool fHardened)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
        LogPrintf("%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (fHardened
        && IsLocked())
        return errorN(1, "%s Wallet must be unlocked to derive hardened keys.", __func__);

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    CExtKeyAccount *sea = mi->second;
    CStoredExtKey *sek = NULL;

    uint32_t nExtKey = fInternal ? sea->nActiveInternal : sea->nActiveExternal;

    if (nExtKey < sea->vExtKeys.size())
        sek = sea->vExtKeys[nExtKey];

    if (!sek)
        return errorN(1, "%s Unknown chain.", __func__);

    uint32_t nChildBkp = fHardened ? sek->nHGenerated : sek->nGenerated;
    uint32_t nChildOut = 0;
    if (0 != sek->DeriveNextKey(pkOut, nChildOut, fHardened))
        return errorN(1, "%s Derive failed.", __func__);

    CEKAKey ks(nExtKey, nChildOut);
    CKeyID idKey = pkOut.GetID();

    bool fUpdateAcc;
    if (0 != ExtKeyAppendToPack(pwdb, sea, idKey, ks, fUpdateAcc))
    {
        sek->SetCounter(nChildBkp, fHardened);
        return errorN(1, "%s ExtKeyAppendToPack failed.", __func__);
    };

    if (!pwdb->WriteExtKey(sea->vExtKeyIDs[nExtKey], *sek))
    {
        sek->SetCounter(nChildBkp, fHardened);
        return errorN(1, "%s Save account chain failed.", __func__);
    };

    if (fUpdateAcc)
    {
        CKeyID idAccount = sea->GetID();
        if (!pwdb->WriteExtAccount(idAccount, *sea))
        {
            sek->SetCounter(nChildBkp, fHardened);
            return errorN(1, "%s Save account chain failed.", __func__);
        };
    };

    sea->SaveKey(idKey, ks); // remove from lookahead, add to pool, add new lookahead

    return 0;
};

int CWallet::NewKeyFromAccount(CPubKey &pkOut, bool fInternal, bool fHardened)
{
    LOCK(cs_wallet);
    CWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != NewKeyFromAccount(&wdb, idDefaultAccount, pkOut, fInternal, fHardened))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);

    return 0;
};

int CWallet::NewStealthKeyFromAccount(CWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel, CEKAStealthKey &akStealthOut)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
        LogPrintf("%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "%s Wallet must be unlocked to derive hardened keys.", __func__);

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    CExtKeyAccount *sea = mi->second;
    uint32_t nChain = sea->nActiveStealth;
    if (nChain >= sea->vExtKeys.size())
        return errorN(1, "%s Stealth chain unknown %d.", __func__, nChain);

    CStoredExtKey *sek = sea->vExtKeys[nChain];


    // - scan secrets must be stored uncrypted - always derive hardened keys

    uint32_t nChildBkp = sek->nHGenerated;

    CKey kScan, kSpend;
    uint32_t nScanOut = 0, nSpendOut = 0;
    if (0 != sek->DeriveNextKey(kScan, nScanOut, true))
        return errorN(1, "%s Derive failed.", __func__);

    if (0 != sek->DeriveNextKey(kSpend, nSpendOut, true))
    {
        sek->SetCounter(nChildBkp, true);
        return errorN(1, "%s Derive failed.", __func__);
    };

    CEKAStealthKey aks(nChain, nScanOut, kScan, nChain, nSpendOut, kSpend);
    aks.sLabel = sLabel;

    std::vector<CEKAStealthKeyPack> aksPak;

    CKeyID idKey = aks.GetID();
    sea->mapStealthKeys[idKey] = aks;

    if (!pwdb->ReadExtStealthKeyPack(idAccount, sea->nPackStealth, aksPak))
    {
        // -- new pack
        aksPak.clear();
        if (fDebug)
            LogPrintf("Account %s, starting new stealth keypack %u.\n", idAccount.ToString(), sea->nPackStealth);
    };

    aksPak.push_back(CEKAStealthKeyPack(idKey, aks));

    if (!pwdb->WriteExtStealthKeyPack(idAccount, sea->nPackStealth, aksPak))
    {
        sea->mapStealthKeys.erase(idKey);
        sek->SetCounter(nChildBkp, true);
        return errorN(1, "%s Save key pack %u failed.", __func__, sea->nPackStealth);
    };

    if ((uint32_t)aksPak.size() >= MAX_KEY_PACK_SIZE-1)
        sea->nPackStealth++;

    if (!pwdb->WriteExtKey(sea->vExtKeyIDs[nChain], *sek))
    {
        sea->mapStealthKeys.erase(idKey);
        sek->SetCounter(nChildBkp, true);
        return errorN(1, "%s Save account chain failed.", __func__);
    };

    bool fOwned = true;
    CStealthAddress sxAddr;
    if (0 != aks.SetSxAddr(sxAddr))
        return errorN(1, "%s SetSxAddr failed.", __func__);
    NotifyAddressBookChanged(this, sxAddr, sLabel, fOwned, CT_NEW, true);


    akStealthOut = aks;
    return 0;
};

int CWallet::NewStealthKeyFromAccount(std::string &sLabel, CEKAStealthKey &akStealthOut)
{
    LOCK(cs_wallet);
    CWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != NewStealthKeyFromAccount(&wdb, idDefaultAccount, sLabel, akStealthOut))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);

    return 0;
};

int CWallet::NewExtKeyFromAccount(CWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel, CStoredExtKey *sekOut)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
        LogPrintf("%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "%s Wallet must be unlocked to derive hardened keys.", __func__);


    bool fHardened = false; // TODO: make option

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    CExtKeyAccount *sea = mi->second;

    CStoredExtKey *sekAccount = sea->ChainAccount();
    if (!sekAccount)
        return errorN(1, "%s Unknown chain.", __func__);

    std::vector<uint8_t> vAccountPath, v;
    mapEKValue_t::iterator miV = sekAccount->mapValue.find(EKVT_PATH);
    if (miV != sekAccount->mapValue.end())
        vAccountPath = miV->second;

    CExtKey evNewKey;

    uint32_t nOldGen = sekAccount->GetCounter(fHardened);
    uint32_t nNewChildNo = 0;
    if (sekAccount->DeriveNextKey(evNewKey, nNewChildNo, fHardened) != 0)
        return errorN(1, "DeriveNextKey failed.");

    sekOut->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT;
    sekOut->kp = evNewKey;
    sekOut->mapValue[EKVT_PATH] = PushUInt32(vAccountPath, nNewChildNo);
    sekOut->mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sekOut->sLabel = sLabel;

    if (IsCrypted()
        && ExtKeyEncrypt(sekOut, vMasterKey, false) != 0)
    {
        sekAccount->SetCounter(nOldGen, fHardened);
        return errorN(1, "ExtKeyEncrypt failed.");
    };

    size_t chainNo = sea->vExtKeyIDs.size();
    CKeyID idNewChain = sekOut->GetID();
    sea->vExtKeyIDs.push_back(idNewChain);
    sea->vExtKeys.push_back(sekOut);

    if (!pwdb->WriteExtAccount(idAccount, *sea)
        || !pwdb->WriteExtKey(idAccount, *sekAccount)
        || !pwdb->WriteExtKey(idNewChain, *sekOut))
    {
        sekAccount->SetCounter(nOldGen, fHardened);
        return errorN(1, "DB Write failed.");
    };

    sea->AddLookAhead(chainNo, N_DEFAULT_LOOKAHEAD);
    mapExtKeys[idNewChain] = sekOut;

    return 0;
};

int CWallet::NewExtKeyFromAccount(std::string &sLabel, CStoredExtKey *sekOut)
{
    LOCK(cs_wallet);
    CWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != NewExtKeyFromAccount(&wdb, idDefaultAccount, sLabel, sekOut))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);

    return 0;
};


int CWallet::ExtKeyGetDestination(const CExtKeyPair &ek, CScript &scriptPubKeyOut, uint32_t &nKey)
{
    if (fDebug)
    {
        CExtKey58 ek58;
        ek58.SetKeyP(ek);
        LogPrintf("%s: %s.\n", __func__, ek58.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    /*
        get the next destination,
        if key is not saved yet, return 1st key
        don't save key here, save after derived key has been sucessfully used
    */


    CKeyID keyId = ek.GetID();

    CWalletDB wdb(strWalletFile, "r+");

    CPubKey pkDest;
    CStoredExtKey sek;
    if (wdb.ReadExtKey(keyId, sek))
    {
        if (0 != sek.DeriveNextKey(pkDest, nKey))
            return errorN(1, "%s: DeriveNextKey failed.", __func__);
        scriptPubKeyOut.SetDestination(pkDest.GetID());
        return 0;
    } else
    {
        nKey = 0; // AddLookAhead starts from 0
        for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
        {
            if (ek.Derive(pkDest, nKey))
            {
                scriptPubKeyOut.SetDestination(pkDest.GetID());
                return 0;
            };
            nKey++;
        };
    };

    return errorN(1, "%s: Could not derive key.", __func__);
};

int CWallet::ExtKeyUpdateLooseKey(const CExtKeyPair &ek, uint32_t nKey, bool fAddToAddressBook)
{
    if (fDebug)
    {
        CExtKey58 ek58;
        ek58.SetKeyP(ek);
        LogPrintf("%s %s.\n", __func__, ek58.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    CKeyID keyId = ek.GetID();

    CWalletDB wdb(strWalletFile, "r+");

    CStoredExtKey sek;
    if (wdb.ReadExtKey(keyId, sek))
    {
        sek.nGenerated = nKey;
        if (!wdb.WriteExtKey(keyId, sek))
            return errorN(1, "%s: WriteExtKey failed.", __func__);
    } else
    {
        sek.kp = ek;
        sek.nGenerated = nKey;
        if (0 != ExtKeyImportLoose(&wdb, sek, false, false))
            return errorN(1, "%s: ExtKeyImportLoose failed.", __func__);
    };

    if (fAddToAddressBook
        && !mapAddressBook.count(CTxDestination(ek)))
    {
        SetAddressBookName(ek, "");
    };
    return 0;
};

bool CWallet::HaveKey(const CKeyID &address) const
{
    //AssertLockHeld(cs_wallet);
    LOCK(cs_wallet);

    CEKAKey ak;
    int rv;
    ExtKeyAccountMap::const_iterator it;
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
    {
        rv = it->second->HaveKey(address, true, ak);
        if (rv == 1)
            return true;
        if (rv == 3)
        {
            if (0 != ExtKeySaveKey(it->second, address, ak))
                return error("HaveKey() ExtKeySaveKey failed.");
            return true;
        };
    };

    return CCryptoKeyStore::HaveKey(address);
};

bool CWallet::HaveExtKey(const CKeyID &keyID) const
{
    LOCK(cs_wallet);

    // NOTE: This only checks keys currently in memory (mapExtKeys)
    //       There may be other extkeys in the db.

    ExtKeyMap::const_iterator it = mapExtKeys.find(keyID);
    if (it != mapExtKeys.end())
        return true;

    return false;
};

bool CWallet::GetKey(const CKeyID &address, CKey &keyOut) const
{
    //AssertLockHeld(cs_wallet);
    LOCK(cs_wallet);

    ExtKeyAccountMap::const_iterator it;
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
    {
        if (it->second->GetKey(address, keyOut))
            return true;
    };

    return CCryptoKeyStore::GetKey(address, keyOut);
};

bool CWallet::GetPubKey(const CKeyID &address, CPubKey& pkOut) const
{
    LOCK(cs_wallet);
    ExtKeyAccountMap::const_iterator it;
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
    {
        if (it->second->GetPubKey(address, pkOut))
            return true;
    };

    return CCryptoKeyStore::GetPubKey(address, pkOut);
};

bool CWallet::HaveStealthAddress(const CStealthAddress &sxAddr) const
{
    if (fDebug)
    {
        AssertLockHeld(cs_wallet);
    };

    if (stealthAddresses.count(sxAddr))
        return true;

    CKeyID sxId = CPubKey(sxAddr.scan_pubkey).GetID();

    ExtKeyAccountMap::const_iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;

        if (ea->mapStealthKeys.size() < 1)
            continue;

        AccStealthKeyMap::iterator it = ea->mapStealthKeys.find(sxId);
        if (it != ea->mapStealthKeys.end())
            return true;
    };

    return false;
};

int CWallet::ScanChainFromTime(int64_t nTimeStartScan)
{
    LogPrintf("%s: %d\n", __func__, nTimeStartScan);


    if (nNodeMode != NT_FULL)
        return errorN(1, "%s: Can't run in thin mode.", __func__);

    CBlockIndex *pindex = pindexGenesisBlock;

    if (pindex == NULL)
        return errorN(1, "%s: Genesis Block is not set.", __func__);

    while (pindex && pindex->nTime < nTimeStartScan && pindex->pnext)
        pindex = pindex->pnext;

    LogPrintf("%s: Starting from height %d.", __func__, pindex->nHeight);

    {
        LOCK2(cs_main, cs_wallet);

        MarkDirty();

        ScanForWalletTransactions(pindex, true);
        ReacceptWalletTransactions();
    } // cs_main, cs_wallet

    return 0;
};

/*------------------------------------------------------------------------------
    CReserveKey
------------------------------------------------------------------------------*/

bool CReserveKey::GetReservedKey(CPubKey& pubkey)
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex != -1)
        {
            vchPubKey = keypool.vchPubKey;
        } else
        {
            LogPrintf("is valid: %d", pwallet->vchDefaultKey.IsValid());
            if (pwallet->vchDefaultKey.IsValid())
            {
                LogPrintf("CReserveKey::GetReservedKey(): Warning: Using default key instead of a new key, top up your keypool!");
                vchPubKey = pwallet->vchDefaultKey;
            } else
                return false;
        };
    };

    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CWallet::GetAllReserveKeys(set<CKeyID>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    BOOST_FOREACH(const int64_t& id, setKeyPool)
    {
        CKeyPool keypool;
        if (!walletdb.ReadPool(id, keypool))
            throw runtime_error("GetAllReserveKeyHashes() : read failed");
        assert(keypool.vchPubKey.IsValid());
        CKeyID keyID = keypool.vchPubKey.GetID();
        if (!HaveKey(keyID))
            throw runtime_error("GetAllReserveKeyHashes() : unknown key in key pool");
        setAddress.insert(keyID);
    };
}

void CWallet::UpdatedTransaction(const uint256 &hashTx)
{
    {
        LOCK(cs_wallet);
        // Only notify UI if this transaction is in this wallet
        WalletTxMap::const_iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
            NotifyTransactionChanged(this, hashTx, CT_UPDATED);
    }
}

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    mapKeyBirth.clear();

    // get birth times for keys with metadata
    for (std::map<CKeyID, CKeyMetadata>::const_iterator it = mapKeyMetadata.begin(); it != mapKeyMetadata.end(); it++)
        if (it->second.nCreateTime)
            mapKeyBirth[it->first] = it->second.nCreateTime;

    // map in which we'll infer heights of other keys
    CBlockIndex *pindexMax = FindBlockByHeight(std::max(0, nBestHeight - 144)); // the tip can be reorganised; use a 144-block safety margin
    std::map<CKeyID, CBlockIndex*> mapKeyFirstBlock;
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);

    BOOST_FOREACH(const CKeyID &keyid, setKeys)
    {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = pindexMax;
    };

    setKeys.clear();

    // if there are no such keys, we're done
    if (mapKeyFirstBlock.empty())
        return;

    // find first block that affects those keys, if there are any left
    std::vector<CKeyID> vAffected;
    for (WalletTxMap::const_iterator it = mapWallet.begin(); it != mapWallet.end(); it++)
    {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = (*it).second;
        std::map<uint256, CBlockIndex*>::const_iterator blit = mapBlockIndex.find(wtx.hashBlock);
        if (blit != mapBlockIndex.end() && blit->second->IsInMainChain())
        {
            // ... which are already in a block
            int nHeight = blit->second->nHeight;
            BOOST_FOREACH(const CTxOut &txout, wtx.vout)
            {
                // iterate over all their outputs
                ::ExtractAffectedKeys(*this, txout.scriptPubKey, vAffected);
                BOOST_FOREACH(const CKeyID &keyid, vAffected)
                {
                    // ... and all their affected keys
                    std::map<CKeyID, CBlockIndex*>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && nHeight < rit->second->nHeight)
                        rit->second = blit->second;
                };
                vAffected.clear();
            };
        };
    };

    // Extract block timestamps for those keys
    for (std::map<CKeyID, CBlockIndex*>::const_iterator it = mapKeyFirstBlock.begin(); it != mapKeyFirstBlock.end(); it++)
        mapKeyBirth[it->first] = it->second->nTime - 7200; // block times can be 2h off
}

bool IsDestMine(const CWallet &wallet, const CTxDestination &dest)
{
    return boost::apply_visitor(CWalletIsMineVisitor(&wallet), dest);
};

static unsigned int HaveKeys(const vector<valtype>& pubkeys, const CWallet& wallet)
{
    unsigned int nResult = 0;
    BOOST_FOREACH(const valtype& pubkey, pubkeys)
    {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (wallet.HaveKey(keyID))
            ++nResult;
    }
    return nResult;
}

bool IsMine(const CWallet &wallet, const CScript& scriptPubKey)
{
    vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    CKeyID keyID;
    switch (whichType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
        return false;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        return wallet.HaveKey(keyID);
    case TX_PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        return wallet.HaveKey(keyID);
    case TX_SCRIPTHASH:
    {
        CScript subscript;
        if (!wallet.GetCScript(CScriptID(uint160(vSolutions[0])), subscript))
            return false;
        return IsMine(wallet, subscript);
    }
    case TX_MULTISIG:
    {
        // Only consider transactions "mine" if we own ALL the
        // keys involved. multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        return HaveKeys(keys, wallet) == keys.size();
    }
    }
    return false;
}

int SetupWalletData(const std::string& strWalletFile, const std::string& sBip44Key, const SecureString& strWalletPassphrase)
{
    if (boost::filesystem::exists(GetDataDir() / strWalletFile))
    {
        return errorN(1, "Wallet file already exists.");
    }

    CWallet wallet(strWalletFile);
    {
        CWalletDB wdb(strWalletFile, "cr+");
        if (wallet.ExtKeyCreateInitial(&wdb, sBip44Key) != 0)
            return errorN(2, "ExtKeyCreateInitial failed.");
    }

    if (!wallet.EncryptWallet(strWalletPassphrase)) {
        return errorN(3, "EncryptWallet failed.");
    }
    bitdb.Flush(false);
    return 0;
}

