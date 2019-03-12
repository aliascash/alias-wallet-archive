// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"

#include "base58.h"

#include "spectregui.h"

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase() && !wtx.IsInMainChain()) // Ensures we show generated coins / mined transactions at depth 1
        return false;

    return true;
}

QString TransactionRecord::getTypeLabel(const int &type)
{
    switch(type)
    {
    case RecvWithAddress:
        return SpectreGUI::tr("XSPEC received with");
    case RecvFromOther:
        return SpectreGUI::tr("XSPEC received from");
    case SendToAddress:
    case SendToOther:
        return SpectreGUI::tr("XSPEC sent to");
    case SendToSelf:
        return SpectreGUI::tr("XSPEC sent to self");
    case SendToSelfSPECTRE:
        return SpectreGUI::tr("SPECTRE sent to self");
    case Generated:
        return SpectreGUI::tr("XSPEC Staked");
    case GeneratedDonation:
        return SpectreGUI::tr("XSPEC Donated");
	case GeneratedContribution:
        return SpectreGUI::tr("XSPEC Contributed");
    case GeneratedSPECTRE:
        return SpectreGUI::tr("SPECTRE Staked");
    case GeneratedSPECTREDonation:
        return SpectreGUI::tr("SPECTRE Donated");
    case GeneratedSPECTREContribution:
        return SpectreGUI::tr("SPECTRE Contributed");
    case RecvSpectre:
        return SpectreGUI::tr("SPECTRE received with");
    case SendSpectre:
        return SpectreGUI::tr("SPECTRE sent to");
    case ConvertSPECTREtoXSPEC:
        return SpectreGUI::tr("SPECTRE to XSPEC");
    case ConvertXSPECtoSPECTRE:
        return SpectreGUI::tr("XSPEC to SPECTRE");
    case Other:
        return SpectreGUI::tr("Other");
    default:
        return "";
    }
}

QString TransactionRecord::getTypeShort(const int &type)
{
    switch(type)
    {
    case TransactionRecord::Generated:
    case TransactionRecord::GeneratedSPECTRE:
        return "staked";
    case TransactionRecord::GeneratedDonation:
    case TransactionRecord::GeneratedSPECTREDonation:
        return "donated";
    case TransactionRecord::GeneratedContribution:
    case TransactionRecord::GeneratedSPECTREContribution:
		return "contributed";      
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
    case TransactionRecord::RecvSpectre:
        return "input";
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
    case TransactionRecord::SendSpectre:
        return "output";
    case TransactionRecord::SendToSelf:
    case TransactionRecord::SendToSelfSPECTRE:
    case TransactionRecord::ConvertXSPECtoSPECTRE:
    case TransactionRecord::ConvertSPECTREtoXSPEC:
         return "inout";
    default:
        return "other";
    }
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();

    int64_t nCredSPEC, nCredSpectre;
    wtx.GetCredit(nCredSPEC, nCredSpectre, true);
    int64_t nCredit = nCredSPEC + nCredSpectre;
    int64_t nDebit = wtx.GetDebit();
    int64_t nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    char cbuf[256];

    if (wtx.nVersion == ANON_TXN_VERSION)
    {
        int64_t allFee;
        std::string strSentAccount;
        std::list<std::tuple<CTxDestination, std::vector<CTxDestination>, int64_t, Currency, std::string> > listReceived;
        std::list<std::tuple<CTxDestination, std::vector<CTxDestination>, int64_t, Currency, std::string> > listSent;

        wtx.GetDestinationDetails(listReceived, listSent, allFee, strSentAccount);

        if (wtx.IsAnonCoinStake())
        {      
            if (!listReceived.empty()) // should never be empty
            {
                const auto & [rDestination, rDestSubs, rAmount, rCurrency, rNarration] = listReceived.front();
                TransactionRecord sub = TransactionRecord(hash, nTime, TransactionRecord::GeneratedSPECTRE,
                                                rDestination.type() == typeid(CStealthAddress) ? boost::get<CStealthAddress>(rDestination).Encoded(): "",
                                                rNarration, 0, -allFee, rCurrency, parts.size());

                for (const auto & [destination, destSubs, amount, currency, narration]: listSent)
                {
                    std::string strAddress = CBitcoinAddress(destination).ToString();
                    if (strAddress == Params().GetDevContributionAddress())
                    {
                        sub.address = strAddress;
                        int blockHeight = wtx.GetDepthAndHeightInMainChain().second;
                        if (blockHeight < 0 || blockHeight % 6 == 0)
                            sub.type = TransactionRecord::GeneratedSPECTREContribution;
                        else
                            sub.type = TransactionRecord::GeneratedSPECTREDonation;
                       break;
                    }
                }
                parts.append(sub);
            }
        }
        else if (listReceived.size() > 0 && listSent.size() > 0)
        {
            // Transfer within account
            TransactionRecord::Type trxType = TransactionRecord::SendToSelfSPECTRE;
            const auto & [sDestination, sDestSubs, sAmount, sCurrency, sNarration] = listSent.front();
            const auto & [rDestination, rDestSubs, rAmount, rCurrency, rNarration] = listReceived.front();

            if (sCurrency == XSPEC && rCurrency == SPECTRE)
                trxType = TransactionRecord::ConvertXSPECtoSPECTRE;
            else if (sCurrency == SPECTRE && rCurrency == XSPEC)
                trxType = TransactionRecord::ConvertSPECTREtoXSPEC;

            for (const auto & [destination, destSubs, amount, currency, narration]: listReceived)
                parts.append(TransactionRecord(hash, nTime, trxType,
                        destination.type() == typeid(CStealthAddress) ? boost::get<CStealthAddress>(destination).Encoded(): "",
                        narration, 0, amount, currency, parts.size())
                );
        }
        else
        {
            for (const auto & [destination, destSubs, amount, currency, narration]: listSent)
                parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendSpectre,
                                               destination.type() == typeid(CStealthAddress) ? boost::get<CStealthAddress>(destination).Encoded(): "",
                                               narration, parts.size() == 0 ? -(amount + allFee): -amount, // add trx fees to first trx record
                                               0, currency, parts.size())
                );

            for (const auto & [destination, destSubs, amount, currency, narration]: listReceived)
                parts.append(TransactionRecord(hash, nTime, TransactionRecord::RecvSpectre,
                                               destination.type() == typeid(CStealthAddress) ? boost::get<CStealthAddress>(destination).Encoded(): "",
                                               narration, 0, amount, currency, parts.size())
                );
        }

        return parts;
    }

    if (nNet > 0 || wtx.IsCoinBase() || wtx.IsCoinStake())
    {
        //
        // Credit
        //

        for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
        {
            const CTxOut& txout = wtx.vout[nOut];

            if (wallet->IsMine(txout))
            {
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size(); // sequence number         

                CTxDestination address;

                sub.credit = txout.nValue;
                if (ExtractDestination(txout.scriptPubKey, address) && IsDestMine(*wallet, address))
                {
                    // Received by Bitcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                } else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }

                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                    sub.narration = mi->second;

                if (wtx.IsCoinBase())
                {
                    // Generated (proof-of-work)
                    sub.type = TransactionRecord::Generated;
                };

                if (wtx.IsCoinStake())
                {
                    // Generated (proof-of-stake)
                    sub.credit = nNet > 0 ? nNet : fabs(wtx.GetValueOut() - nDebit);
                    sub.type = TransactionRecord::Generated;

                    // check if stake was contributed
                    for (const auto & txout : wtx.vout)
                    {
                        CTxDestination address;
                        if (ExtractDestination(txout.scriptPubKey, address))
                        {
                            std::string strAddress = CBitcoinAddress(address).ToString();
                            if (strAddress == Params().GetDevContributionAddress())
                            {
                                sub.address = strAddress;
                                int blockHeight = wtx.GetDepthAndHeightInMainChain().second;
                                if (blockHeight < 0 || blockHeight % 6 == 0)
                                    sub.type = TransactionRecord::GeneratedContribution;
                                else
                                    sub.type = TransactionRecord::GeneratedDonation;
                                break;
                            }
                        }
                    }
                    parts.append(sub);
                    break;
                }
                parts.append(sub);
            }
        }
    } else
    {
        bool fAllFromMe = true;
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            if (wallet->IsMine(txin))
                continue;
            fAllFromMe = false;
            break;
        };

        bool fAllToMe = true;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            opcodetype firstOpCode;
            CScript::const_iterator pc = txout.scriptPubKey.begin();
            if (txout.scriptPubKey.GetOp(pc, firstOpCode)
                && firstOpCode == OP_RETURN)
                continue;
            if (wallet->IsMine(txout))
                continue;

            fAllToMe = false;
            break;
        };

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            int64_t nChange = wtx.GetChange();

            std::string narration("");
            mapValue_t::const_iterator mi;
            for (mi = wtx.mapValue.begin(); mi != wtx.mapValue.end(); ++mi)
            {
                if (mi->first.compare(0, 2, "n_") != 0)
                    continue;
                narration = mi->second;
                break;
            };

            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf, "", narration,
                            -(nDebit - nChange), nCredit - nChange, XSPEC));
        } else
        if (fAllFromMe)
        {
            //
            // Debit
            //
            int64_t nTxFee = nDebit - wtx.GetValueOut();


            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];

                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();

                opcodetype firstOpCode;
                CScript::const_iterator pc = txout.scriptPubKey.begin();
                if (txout.scriptPubKey.GetOp(pc, firstOpCode)
                    && firstOpCode == OP_RETURN)
                    continue;

                if (wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Bitcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                } else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                    sub.narration = mi->second;

                int64_t nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                parts.append(sub);
            }
        } else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            TransactionRecord sub(hash, nTime, TransactionRecord::Other, "", "", nNet, 0, XSPEC);
            /*
            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                // display 1st transaction
                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                    sub.narration = mi->second;
                break;
            };
            */
            parts.append(sub);
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    int nHeight = std::numeric_limits<int>::max();

    // Find the block the tx is in
    if (nNodeMode == NT_FULL)
    {
        CBlockIndex* pindex = NULL;
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
        if (mi != mapBlockIndex.end())
        {
            pindex = (*mi).second;
            nHeight = pindex->nHeight;
        };
    } else
    {
        CBlockThinIndex* pindex = NULL;
        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtx.hashBlock);
        if (mi != mapBlockThinIndex.end())
        {
            pindex = (*mi).second;
            nHeight = pindex->nHeight;
        };
    };

    // Sort order nTime has priority (also for unrecorded transactions with nHeight=max)
    // only the first 200 transactions are updated in updateTransactions(),
    // sorted by nTime makes sure the newest trx are considered for update
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
                               wtx.nTime,
                               nHeight,
                               (wtx.IsCoinBase() ? 1 : 0),
                               idx);

    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = nBestHeight;

    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = nBestHeight - wtx.nLockTime;
        } else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        };
    } else
    if (type == TransactionRecord::Generated || type == TransactionRecord::GeneratedSPECTRE ||
            type == TransactionRecord::GeneratedDonation || type == TransactionRecord::GeneratedSPECTREDonation ||
            type == TransactionRecord::GeneratedContribution || type == TransactionRecord::GeneratedSPECTREContribution)
    {
        // For generated transactions, determine maturity
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            } else
                status.status = TransactionStatus::NotAccepted;
        } else
            status.status = TransactionStatus::Confirmed;
    } else
    {
        if (status.depth < 0)
            status.status = TransactionStatus::Conflicted;
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            status.status = TransactionStatus::Offline;
        else if (status.depth == 0)
            status.status = TransactionStatus::Unconfirmed;
        else if (status.depth < RecommendedNumConfirmations)
            status.status = TransactionStatus::Confirming;
        else
            status.status = TransactionStatus::Confirmed;
    }
}

bool TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);

    return status.cur_num_blocks != nBestHeight;
}

std::string TransactionRecord::getTxID()
{
    return hash.ToString() + strprintf("-%03d", idx);
}

QString TransactionRecord::getTypeLabel()
{
    return getTypeLabel(type);
}
