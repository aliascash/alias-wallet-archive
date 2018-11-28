#include "transactionrecord.h"

#include "wallet.h"
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
        return SpectreGUI::tr("XSPEC sent");
    case SendToSelf:
        return SpectreGUI::tr("Payment to yourself");
    case Generated:
        return SpectreGUI::tr("Staked");
    case GeneratedDonation:
        return SpectreGUI::tr("Donated");
	case GeneratedContribution:
		return SpectreGUI::tr("Contributed");
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
        return "staked";
    case TransactionRecord::GeneratedDonation:
        return "donated";
	case TransactionRecord::GeneratedContribution:
		return "contributed";
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
    case TransactionRecord::RecvSpectre:
        return "input";
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
    case TransactionRecord::SendSpectre:
        return "output";
    default:
        return "inout";
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
    uint256 hash = wtx.GetHash(), hashPrev = 0;
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    char cbuf[256];

    const std::string XSPEC = "XSPEC";
    const std::string SPECTRE = "SPECTRE";

    std::string sCurrencyDestination = XSPEC;
    std::string sCurrencySource = XSPEC;
    for(const CTxIn& txin: wtx.vin) {
        if (txin.IsAnonInput() ) {
            sCurrencySource = SPECTRE;
            break;
        }
    }

    if (wtx.nVersion == ANON_TXN_VERSION)
    {
        bool withinAccount = false;
        std::map<std::string, int64_t> mapAddressAmounts;
        std::map<std::string, std::string> mapAddressNarration;
        for (uint32_t index = 0; index < wtx.vout.size(); ++index)
        {
            const CTxOut& txout = wtx.vout[index];
            if (txout.IsAnonOutput())
            {
                // Don't report 'change' txouts
                if (wallet->IsChange(txout))
                    continue;

                sCurrencyDestination = SPECTRE;

                const CScript &s = txout.scriptPubKey;
                CKeyID ckidD = CPubKey(&s[2+1], 33).GetID();

                bool fIsMine = wallet->HaveKey(ckidD);

                std::string account;
                {
                    LOCK(wallet->cs_wallet);
                    if (wallet->mapAddressBook.count(ckidD))
                    {
                        account = wallet->mapAddressBook.at(ckidD);
                    }
                    else {
                        account = "UNKNOWN";
                    }
                }

                if (fIsMine)
                    mapAddressAmounts[account] += txout.nValue;
                else if (nDebit > 0)
                    mapAddressAmounts[account] -= txout.nValue;

                if (fIsMine && nDebit > 0)
                    withinAccount = true;

                // Add narration for account
                snprintf(cbuf, sizeof(cbuf), "n_%u", mapAddressAmounts.size()-1);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty()){
                    mapAddressNarration[account] = mi->second;
                    LogPrintf("mapAccountNarration[%s] = '%s'\n", account, mi->second);
                }
            }
        }
        bool firstSend = true;
        for (const auto & [address, amount] : mapAddressAmounts) {
            int64_t amountAdjusted = amount;
            TransactionRecord::Type trxType = TransactionRecord::RecvSpectre;
            if (withinAccount) {
                if (sCurrencySource == XSPEC && sCurrencyDestination == SPECTRE)
                    trxType = TransactionRecord::ConvertSPECTREtoXSPEC;
                else if (sCurrencySource == SPECTRE && sCurrencyDestination == XSPEC)
                    trxType = TransactionRecord::ConvertSPECTREtoXSPEC;
                else
                    trxType = TransactionRecord::SendToSelf;
            }
            else if (amount < 0) {
                trxType = TransactionRecord::SendSpectre;
                if (firstSend) {
                    // add trx fees to first trx record
                    firstSend = false;
                    int64_t nTxFee = nDebit - wtx.GetValueOut();
                    amountAdjusted = amountAdjusted - nTxFee;
                }
            }
            TransactionRecord sub(hash, nTime, trxType, "", "", amountAdjusted, 0);

            CStealthAddress stealthAddress;
            if (wallet->GetStealthAddress(address, stealthAddress))
                sub.address = stealthAddress.Encoded();

            mapValue_t::const_iterator ni = mapAddressNarration.find(address);
            if (ni != mapAddressNarration.end() && !ni->second.empty()) {
                 sub.narration = ni->second;
                 LogPrintf("sub.narration[%s] = '%s'\n", address, ni->second);
            }

            parts.append(sub);
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

                    if (hashPrev == hash)
                        continue; // last coinstake output

                    sub.credit = nNet > 0 ? nNet : fabs(wtx.GetValueOut() - nDebit);
                    sub.type = TransactionRecord::Generated;
                    hashPrev = hash;
                };

                parts.append(sub);
            }
            else if (wtx.IsCoinStake()) {
                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address)) {
                    std::string strAddress = CBitcoinAddress(address).ToString();
                    if (strAddress == Params().GetDevContributionAddress()) {
                        TransactionRecord sub(hash, nTime);
                        sub.address = strAddress;
						if (wtx.GetDepthAndHeightInMainChain().second % 6 == 0) {
							sub.type = TransactionRecord::GeneratedContribution;
						}
						else {
							sub.type = TransactionRecord::GeneratedDonation;
						}                       
                        sub.credit = txout.nValue;
                        sub.idx = parts.size(); // sequence number
                        parts.append(sub);
                    }
                }
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
                            -(nDebit - nChange), nCredit - nChange));
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
            TransactionRecord sub(hash, nTime, TransactionRecord::Other, "", "", nNet, 0);
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

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        nHeight,
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
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
    if (type == TransactionRecord::Generated || type == TransactionRecord::GeneratedDonation || type == TransactionRecord::GeneratedContribution)
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
