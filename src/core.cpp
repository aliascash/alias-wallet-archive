// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

void addAnonOutput(CPubKey& pkAo, CAnonOutput& anonOutput, txMixins_container& txMixinsContainer)
{
    // create pair with vout index of tx and pubKey
    const auto & pairOutPubkey = std::make_pair(anonOutput.outpoint.n, pkAo);
    // check if CTxMixins does already exist in container for tx hash
    const auto & txHashIndex = txMixinsContainer.get<TXHASH>();
    const auto it = txHashIndex.find(anonOutput.outpoint.hash);
    if (it != txHashIndex.end())
        // add anon pubKey to existing CTxMixins in container
        it->vOutPubKeys.push_back(pairOutPubkey);
    else
    {
        CTxMixins txMixins(anonOutput.outpoint.hash);
        txMixins.vOutPubKeys.push_back(pairOutPubkey);
        txMixinsContainer.push_back(txMixins);
    }
}

void CMixins::AddAnonOutput(CPubKey& pkAo, CAnonOutput& anonOutput, int blockHeight)
{
    CTxMixinsContainers& txMixinsContainers = mapMixins[anonOutput.nValue];

    if (blockHeight - anonOutput.nBlockHeight < 2700) // blocks of last 3 days
        // add anon to recent mixins container
        addAnonOutput(pkAo, anonOutput, txMixinsContainers.get(RECENT));
    else
        // add anon to old mixins container
        addAnonOutput(pkAo, anonOutput, txMixinsContainers.get(OLD));
}

void removeEmptyTx(txMixins_container& txMixinsContainer, std::set<uint64_t>& setPickedTxInd)
{
    // Remove transactions which don't provide any more anons
    for (auto iter = setPickedTxInd.rbegin(); iter != setPickedTxInd.rend(); ++iter)
    {
        const CTxMixins& txMixins = txMixinsContainer.at(*iter);
        if (txMixins.vOutPubKeys.empty())
        {
            if (fDebugRingSig)
                LogPrintf("CMixins::removeEmptyTx() : erase tx %s.\n", txMixins.txHash.ToString());
            txMixinsContainer.erase(txMixinsContainer.begin() + *iter);
        }
    }
}

void pick(std::vector<std::pair<int, uint256>>& vUsedTx, CTxMixinsContainers& txMixinsContainers, int containerId, std::set<uint64_t>& setPickedTxInd, std::vector<CPubKey>& vPickedAnons)
{
    txMixins_container& txMixinsContainer = txMixinsContainers.get(containerId);
    // Pick a random transaction
    uint64_t nAvailableTx = txMixinsContainer.size() - setPickedTxInd.size();
    uint64_t iPickTx = GetRand(nAvailableTx);
    // adust the pick index to skip already used transactions
    for (const auto & iPickedTx : setPickedTxInd)
        if (iPickedTx <= iPickTx)
            iPickTx++;

    const CTxMixins& txMixins = txMixinsContainer.at(iPickTx);
     // Remember for this picking loop, the transaction from which the anon was picked
    setPickedTxInd.insert(iPickTx);

    // Pick random a anon from the picked transaction
    uint64_t iPickAnon = GetRand(txMixins.vOutPubKeys.size());
    const auto & [iVout, pubKey] = txMixins.vOutPubKeys.at(iPickAnon);
    vPickedAnons.push_back(pubKey);
    if (fDebugRingSig)
        LogPrintf("CMixins::pick() : pick mixin %d from %s tx %s vout %d.\n",
                  vPickedAnons.size(), containerId == OLD ? "OLD" : "RECENT", txMixins.txHash.ToString(), iVout);

    // Erase the anon, every anon can only be used once as mixin per transaction
    txMixins.vOutPubKeys.erase(txMixins.vOutPubKeys.begin() + iPickAnon);

    // Remember for CMixins state, the transaction from which the anon was picked
    auto pairContTx = std::make_pair(containerId, txMixins.txHash);
    if (std::find(vUsedTx.begin(), vUsedTx.end(), pairContTx) == vUsedTx.end())
        vUsedTx.push_back(pairContTx);
}


bool CMixins::Pick(int64_t nValue, uint8_t nMixins, std::vector<CPubKey>& vPickedAnons)
{
    int64_t nStart = GetTimeMicros();
    std::set<uint64_t> setPickedTxInd[2];
    CTxMixinsContainers& txMixinsContainers = mapMixins[nValue];
    uint64_t nUsedTx = vUsedTx.size();

    if (fDebugRingSig)
        LogPrintf("CMixins::Pick() : pick %d mixins of value %d from %d recent and %d old transactions. Previously picked txs: %d.\n",
                  nMixins, nValue, txMixinsContainers.get(RECENT).size(), txMixinsContainers.get(OLD).size(), nUsedTx);

    if (txMixinsContainers.get(OLD).size() + txMixinsContainers.get(RECENT).size() < nMixins)
        return false;

    for (uint8_t i = 0; i < nMixins; i++)
    {
        uint64_t mode = GetRand(100);
        if (mode < 33 && nUsedTx > 0)
        {
            // take mixin from used transactions
            if (fDebugRingSig)
                LogPrintf("CMixins::Pick() : try to pick from %d previously picked tx.\n", nUsedTx);
            // shuffle previous picked transaction list to make sure order of tx pick does not reveal its fake nature
            std::shuffle(vUsedTx.begin(), vUsedTx.begin() + nUsedTx, urng);
            bool foundMixin = false;
            uint64_t iUsedTx = 0;
            for (const auto & [iContainer, txHash] : vUsedTx)
            {
                if (iUsedTx++ >= nUsedTx)
                    break;

                txMixins_container& txMixinsContainer = txMixinsContainers.get(iContainer);
                auto & txHashIndex = txMixinsContainer.get<TXHASH>();
                auto it = txHashIndex.find(txHash);
                if (it != txHashIndex.end())
                {
                    // Get index of tx in containers random index
                    uint64_t iPickTx = txMixinsContainer.iterator_to(*it) - txMixinsContainer.begin();

                    // check if tx was already used for this ring sig
                    if (setPickedTxInd[iContainer].find(iPickTx) != setPickedTxInd[iContainer].end())
                        continue;

                    // Pick random a anon from the transacton
                    uint64_t iPickAnon = GetRand(it->vOutPubKeys.size());
                    const auto & [iVout, pubKey] = it->vOutPubKeys.at(iPickAnon);
                    vPickedAnons.push_back(pubKey);
                    if (fDebugRingSig)
                            LogPrintf("CMixins::Pick() : pick mixin %d from prev tx %s vout %d.\n", vPickedAnons.size(), txHash.ToString(), iVout);
                    // Erase the anon, every anon can only be used once as mixin per transaction
                    it->vOutPubKeys.erase(it->vOutPubKeys.begin() + iPickAnon);

                    // Remember for this picking loop, the transaction from which the anon was picked
                    setPickedTxInd[iContainer].insert(iPickTx);
                    foundMixin = true;
                    break;
                }
            }
            if (foundMixin)
                continue;
        }

        uint64_t nAvailableOldTx = txMixinsContainers.get(OLD).size() - setPickedTxInd[OLD].size();
        uint64_t nAvailableRecentTx = txMixinsContainers.get(RECENT).size() - setPickedTxInd[RECENT].size();
        int containerId = nAvailableRecentTx > 0 && (mode >= 66 || nAvailableOldTx == 0) ? RECENT : OLD;
        pick(vUsedTx, txMixinsContainers, containerId, setPickedTxInd[containerId], vPickedAnons);
    }
    // Remove transactions which don't provide any more anons
    removeEmptyTx(txMixinsContainers.get(RECENT), setPickedTxInd[RECENT]);
    removeEmptyTx(txMixinsContainers.get(OLD), setPickedTxInd[OLD]);

    if (fDebugRingSig)
        LogPrintf("CMixins::Pick() : picked %d mixins in %d µs.\n", nMixins, GetTimeMicros() - nStart);
    return true;
}



