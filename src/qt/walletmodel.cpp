// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2011 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "walletmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "interface.h"
#include "wallet.h"
#include "walletdb.h" // for BackupWallet
#include "base58.h"
#include "rpcserver.h"

#include <QSet>
#include <QTimer>
#include <QApplication>
#include <QDebug>
#include <QFile>

WalletModel::WalletModel(CWallet *wallet, OptionsModel *optionsModel, QObject *parent) :
    WalletModelRemoteSimpleSource(parent), wallet(wallet), optionsModel(optionsModel), addressTableModel(0),
    transactionTableModel(0),
    cachedBalance(0), cachedSpectreBal(0), cachedStake(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0),
    cachedNumTransactions(0),
    cachedNumBlocks(0),
    fUnlockRescanRequested(false),
    fForceCheckBalanceChanged(false)
{
    addressTableModel = new AddressTableModel(wallet, this);
    transactionTableModel = new TransactionTableModel(wallet, this);

    updateStatus(true);

    subscribeToCoreSignals();

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
}

qint64 WalletModel::getBalance() const
{
    return wallet->GetBalance();
}

qint64 WalletModel::getSpectreBalance() const
{
    return wallet->GetSpectreBalance();
}

qint64 WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

qint64 WalletModel::getUnconfirmedSpectreBalance() const
{
    return wallet->GetUnconfirmedSpectreBalance();
}

qint64 WalletModel::getStake() const
{
    return wallet->GetStake();
}

qint64 WalletModel::getSpectreStake() const
{
    return wallet->GetSpectreStake();
}

qint64 WalletModel::getImmatureBalance() const
{
    return wallet->GetImmatureBalance();
}

qint64 WalletModel::getImmatureSpectreBalance() const
{
    return wallet->GetImmatureSpectreBalance();
}

int WalletModel::getNumTransactions() const
{
    int numTransactions = 0;
    {
        LOCK(wallet->cs_wallet);
        numTransactions = wallet->mapWallet.size();
    }
    return numTransactions;
}

void WalletModel::updateStatus(bool force)
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(force || encryptionInfo().status() != newEncryptionStatus || encryptionInfo().fWalletUnlockStakingOnly() != fWalletUnlockStakingOnly)
    {
        fForceCheckBalanceChanged = true;
        setEncryptionInfo(EncryptionInfo(newEncryptionStatus, fWalletUnlockStakingOnly));
        emit encryptionStatusChanged(newEncryptionStatus);
    }
}

void WalletModel::pollBalanceChanged()
{
    // Get required locks upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(cs_main, lockMain);
    if(!lockMain)
        return;
    TRY_LOCK(wallet->cs_wallet, lockWallet);
    if(!lockWallet)
        return;

    if(fForceCheckBalanceChanged || nBestHeight != cachedNumBlocks || fIsStaking != stakingInfo().fIsStaking())
        updateStakingInfo();

    if(fForceCheckBalanceChanged || nBestHeight != cachedNumBlocks)
    {
        fForceCheckBalanceChanged = false;

        int newNumTransactions = getNumTransactions();
        if(cachedNumTransactions != newNumTransactions)
        {
            cachedNumTransactions = newNumTransactions;
            emit numTransactionsChanged(newNumTransactions);
        }

        // Balance and number of transactions might have changed
        cachedNumBlocks = nBestHeight;

        if(transactionTableModel)
            transactionTableModel->updateConfirmations();

        updateStakingInfo();
    }
}


void WalletModel::checkBalanceChanged(bool force)
{
    qint64 newBalance = getBalance();
    qint64 newSpectreBal = getSpectreBalance();
    qint64 newStake = getStake();
    qint64 newSpectreStake = getSpectreStake();
    qint64 newUnconfirmedBalance = getUnconfirmedBalance();
    qint64 newUnconfirmedSpectreBalance = getUnconfirmedSpectreBalance();
    qint64 newImmatureBalance = getImmatureBalance();
    qint64 newImmatureSpectreBalance = getImmatureSpectreBalance();

    if (cachedBalance != newBalance
            || cachedSpectreBal != newSpectreBal
            || cachedStake != newStake
            || cachedSpectreStake != newSpectreStake
            || cachedUnconfirmedBalance != newUnconfirmedBalance
            || cachedUnconfirmedSpectreBalance != newUnconfirmedSpectreBalance
            || cachedImmatureBalance != newImmatureBalance
            || cachedImmatureSpectreBalance != newImmatureSpectreBalance
            || force == true)
    {
        cachedBalance = newBalance;
        cachedSpectreBal = newSpectreBal;
        cachedStake = newStake;
        cachedSpectreStake = newSpectreStake;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedUnconfirmedSpectreBalance = newUnconfirmedSpectreBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedImmatureSpectreBalance = newImmatureSpectreBalance;
        emit balanceChanged(newBalance, newSpectreBal, newStake, newSpectreStake, newUnconfirmedBalance, newUnconfirmedSpectreBalance, newImmatureBalance, cachedImmatureSpectreBalance);
    }
}

void WalletModel::updateStakingInfo()
{
    double nWeight = 0, nNetworkWeight = GetPoSKernelPS(), nNetworkWeightRecent = GetPoSKernelPSRecent();
    unsigned nEstimateTime = 0;

    if (fIsStaking)
    {
        nWeight = wallet->GetStakeWeight() + wallet->GetSpectreStakeWeight();
        if (nWeight)
            nEstimateTime = GetTargetSpacing(nBestHeight, GetAdjustedTime()) * nNetworkWeight / nWeight;
    }
    setStakingInfo(StakingInfo(fIsStaking, nWeight, nEstimateTime, nNetworkWeight, nNetworkWeightRecent));
}




void WalletModel::updateTransaction(const QString &hash, int status)
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
}

void WalletModel::updateAddressBook(const QString &address, const QString &label, bool isMine, int status, bool fManual)
{
    if (fManual)
        fPassGuiAddresses = true;
    if (addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, status);
    fPassGuiAddresses = false;
}

bool WalletModel::validateAddress(const QString &address)
{
    std::string sAddr = address.toStdString();

    if (address.length() > 75 && IsBIP32(sAddr.c_str())) // < 75, don't bother checking plain addrs
        return true;

    if (address.length() > 75)
        return IsStealthAddress(sAddr); // > 75, will never be a plain address, exit here

    CBitcoinAddress addressParsed(sAddr);
    return addressParsed.IsValid();
}

SendCoinsReturn status(SendCoinsStatusEnum::SendCoinsStatus status)
{
    return SendCoinsReturn(status, 0, QString());
}

SendCoinsReturn WalletModel::sendCoins(const qint64 feeApproval, const QList<SendCoinsRecipient> &recipients, const QList<OutPoint> &coins)
{
    const CCoinControl *coinControl = nullptr;
    qint64 total = 0;
    QSet<QString> setAddress;
    QString hex;

    if(recipients.empty())
        return status(SendCoinsStatusEnum::OK);

    // Pre-check input data for validity
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        if(!validateAddress(rcp.address()))
            return status(SendCoinsStatusEnum::InvalidAddress);

        if(IsStealthAddress(rcp.address().toStdString()))
            return status(SendCoinsStatusEnum::StealthAddressOnlyAllowedForSPECTRE);

        setAddress.insert(rcp.address());

        if(rcp.amount() <= 0)
            return status(SendCoinsStatusEnum::InvalidAmount);

        total += rcp.amount();
    }

    if (recipients.size() > setAddress.size())
        foreach(QString rcpAddr, setAddress)
            if(!IsStealthAddress(rcpAddr.toStdString()))
                return status(SendCoinsStatusEnum::DuplicateAddress);

    int64_t nBalance = 0;
    std::vector<COutput> vCoins;
    wallet->AvailableCoins(vCoins, true, coinControl);

    BOOST_FOREACH(const COutput& out, vCoins)
        nBalance += out.tx->vout[out.i].nValue;

    if(total > nBalance)
        return status(SendCoinsStatusEnum::AmountExceedsBalance);

    std::map<int, std::string> mapStealthNarr;

    {
        LOCK2(cs_main, wallet->cs_wallet);

        CWalletTx wtx;

        // Sendmany
        std::vector<std::pair<CExtKeyPair, uint32_t> > vecUpdate;
        std::vector<std::pair<CScript, int64_t> > vecSend;
        foreach(const SendCoinsRecipient &rcp, recipients)
        {
            std::string sAddr = rcp.address().toStdString();

            if (rcp.addressTypeInd() == AT_Stealth)
            {
                CStealthAddress sxAddr;
                if (sxAddr.SetEncoded(sAddr))
                {
                    ec_secret ephem_secret;
                    ec_secret secretShared;
                    ec_point pkSendTo;
                    ec_point ephem_pubkey;


                    if (GenerateRandomSecret(ephem_secret) != 0)
                    {
                        LogPrintf("GenerateRandomSecret failed.\n");
                        return status(SendCoinsStatusEnum::Aborted);
                    };

                    if (StealthSecret(ephem_secret, sxAddr.scan_pubkey, sxAddr.spend_pubkey, secretShared, pkSendTo) != 0)
                    {
                        LogPrintf("Could not generate receiving public key.\n");
                        return status(SendCoinsStatusEnum::Aborted);
                    };

                    CPubKey cpkTo(pkSendTo);
                    if (!cpkTo.IsValid())
                    {
                        LogPrintf("Invalid public key generated.\n");
                        return status(SendCoinsStatusEnum::Aborted);
                    };

                    CBitcoinAddress addrTo(cpkTo.GetID());

                    if (SecretToPublicKey(ephem_secret, ephem_pubkey) != 0)
                    {
                        LogPrintf("Could not generate ephem public key.\n");
                        return status(SendCoinsStatusEnum::Aborted);
                    };

                    if (fDebug)
                    {
                        LogPrintf("Stealth send to generated pubkey %u: %s\n", pkSendTo.size(), HexStr(pkSendTo).c_str());
                        LogPrintf("hash %s\n", addrTo.ToString().c_str());
                        LogPrintf("ephem_pubkey %u: %s\n", ephem_pubkey.size(), HexStr(ephem_pubkey).c_str());
                    };

                    CScript scriptPubKey;
                    scriptPubKey.SetDestination(addrTo.Get());

                    vecSend.push_back(make_pair(scriptPubKey, rcp.amount()));

                    CScript scriptP = CScript() << OP_RETURN << ephem_pubkey;

                    if (rcp.narration().length() > 0)
                    {
                        std::string sNarr = rcp.narration().toStdString();

                        if (sNarr.length() > 24)
                        {
                            LogPrintf("Narration is too long.\n");
                            return status(SendCoinsStatusEnum::NarrationTooLong);
                        };

                        std::vector<unsigned char> vchNarr;

                        SecMsgCrypter crypter;
                        crypter.SetKey(&secretShared.e[0], &ephem_pubkey[0]);

                        if (!crypter.Encrypt((uint8_t*)&sNarr[0], sNarr.length(), vchNarr))
                        {
                            LogPrintf("Narration encryption failed.\n");
                            return status(SendCoinsStatusEnum::Aborted);
                        };

                        if (vchNarr.size() > 48)
                        {
                            LogPrintf("Encrypted narration is too long.\n");
                            return status(SendCoinsStatusEnum::Aborted);
                        };

                        if (vchNarr.size() > 0)
                            scriptP = scriptP << OP_RETURN << vchNarr;

                        int pos = vecSend.size()-1;
                        mapStealthNarr[pos] = sNarr;
                    };

                    vecSend.push_back(std::make_pair(scriptP, 0));
                    continue;
                }; // else drop through to normal
            }

            // TODO: why the drop through for stealth?!
            CScript scriptPubKey;

            if (rcp.addressTypeInd() == AT_BIP32)
            {
                CBitcoinAddress address(sAddr);
                CTxDestination dest = address.Get();
                if (dest.type() != typeid(CExtKeyPair))
                {
                    LogPrintf("Error: Address is not an extended address.\n");
                    return status(SendCoinsStatusEnum::Aborted);
                };

                CExtKeyPair ek = boost::get<CExtKeyPair>(dest);
                CExtKey58 ek58;
                ek58.SetKeyP(ek);
                uint32_t nChildKey;
                if (0 != wallet->ExtKeyGetDestination(ek, scriptPubKey, nChildKey))
                    return status(SendCoinsStatusEnum::InvalidAddress);

                vecUpdate.push_back(std::make_pair(ek, nChildKey));
            } else
            {
                scriptPubKey.SetDestination(CBitcoinAddress(sAddr).Get());
            };

            vecSend.push_back(make_pair(scriptPubKey, rcp.amount()));

            if (rcp.narration().length() > 0)
            {
                std::string sNarr = rcp.narration().toStdString();

                if (sNarr.length() > 24)
                {
                    LogPrintf("Narration is too long.\n");
                    return status(SendCoinsStatusEnum::NarrationTooLong);
                };

                std::vector<uint8_t> vNarr(sNarr.c_str(), sNarr.c_str() + sNarr.length());
                std::vector<uint8_t> vNDesc;

                vNDesc.resize(2);
                vNDesc[0] = 'n';
                vNDesc[1] = 'p';

                CScript scriptN = CScript() << OP_RETURN << vNDesc << OP_RETURN << vNarr;

                vecSend.push_back(make_pair(scriptN, 0));
            }
        }

        int64_t nFeeRequired = 0;
        int nChangePos = -1;
        bool fCreated = wallet->CreateTransaction(vecSend, wtx, nFeeRequired, nChangePos, coinControl);

        std::map<int, std::string>::iterator it;
        for (it = mapStealthNarr.begin(); it != mapStealthNarr.end(); ++it)
        {
            int pos = it->first;
            if (nChangePos > -1 && it->first >= nChangePos)
                pos++;

            char key[64];
            if (snprintf(key, sizeof(key), "n_%u", pos) < 1)
            {
                LogPrintf("CreateStealthTransaction(): Error creating narration key.");
                continue;
            };
            wtx.mapValue[key] = it->second;
        };


        if (!fCreated)
        {
            if ((total + nFeeRequired) > nBalance) // FIXME: could cause collisions in the future
                return SendCoinsReturn(SendCoinsStatusEnum::AmountWithFeeExceedsBalance, nFeeRequired, QString());

            return status(SendCoinsStatusEnum::TransactionCreationFailed);
        }
//        if (!uiInterface.ThreadSafeAskFee(nFeeRequired, tr("Sending...").toStdString()))
//            return status(SendCoinsStatusEnum::Aborted);
//        if (feeApproval != nFeeRequired)
//            return SendCoinsReturn(SendCoinsStatusEnum::ApproveFee, nFeeRequired, QString());

        std::map<CKeyID, CStealthAddress> mapPubStealth;
        if (!wallet->CommitTransaction(wtx, &mapPubStealth))
            return status(SendCoinsStatusEnum::TransactionCommitFailed);

        hex = QString::fromStdString(wtx.GetHash().GetHex());

        // - Update sent to ext keys
        for (uint32_t k = 0; k < vecUpdate.size(); ++k)
        {
            wallet->ExtKeyUpdateLooseKey(vecUpdate[k].first, vecUpdate[k].second, false); // mapAddressBook will be added after this
        };
    } // cs_main, wallet->cs_wallet

    // Add addresses / update labels that we've sent to to the address book
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        if(rcp.label().isEmpty()) // Don't add addresses without labels...
            continue;

        std::string strAddress = rcp.address().toStdString();
        std::string strLabel = rcp.label().toStdString();

        {
            LOCK(wallet->cs_wallet);

            if (rcp.addressTypeInd() == AT_Stealth)
            {
                wallet->UpdateStealthAddress(strAddress, strLabel, true);
            } else
            {
                CTxDestination dest = CBitcoinAddress(strAddress).Get();
                std::map<CTxDestination, std::string>::iterator mi = wallet->mapAddressBook.find(dest);

                // Check if we have a new address or an updated label
                if (mi == wallet->mapAddressBook.end() || mi->second != strLabel)
                    wallet->SetAddressBookName(dest, strLabel);
            };
        } // wallet->cs_wallet
    };

    return SendCoinsReturn(SendCoinsStatusEnum::OK, 0, hex);
}

SendCoinsReturn WalletModel::sendCoinsAnon(const qint64 feeApproval, const QList<SendCoinsRecipient> &recipients, const QList<OutPoint> &coins)
{
    const CCoinControl *coinControl = nullptr;

    if (fDebugRingSig)
        LogPrintf("sendCoinsAnon()\n");

    int64_t nTotalOut = 0;
    int64_t nBalance = 0;
    int64_t nFeeRequired = 0;
    QString hex;

    if (recipients.empty())
        return status(SendCoinsStatusEnum::OK);

    if (nNodeMode != NT_FULL)
        return status(SendCoinsStatusEnum::SCR_NeedFullMode);

    if (nBestHeight < GetNumBlocksOfPeers()-1)
        return SendCoinsReturn(SendCoinsStatusEnum::SCR_ErrorWithMsg, 0, QString::fromStdString("Block chain must be fully synced first."));

    if (vNodes.empty())
        return SendCoinsReturn(SendCoinsStatusEnum::SCR_ErrorWithMsg, 0, QString::fromStdString("Alias is not connected!"));


    // -- verify input type and ringsize
    int inputTypes = -1;
    int nRingSize = -1;
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        int inputType = 0;
        switch(rcp.txnType())
        {
            case TxnTypeEnum::TXT_SPEC_TO_SPEC:
            case TxnTypeEnum::TXT_SPEC_TO_ANON:
                inputType = 0;
                break;
            case TxnTypeEnum::TXT_ANON_TO_ANON:
            case TxnTypeEnum::TXT_ANON_TO_SPEC:
                inputType = 1;
                break;
            default:
                return status(SendCoinsStatusEnum::InputTypeError);
        };

        if (inputTypes == -1)
            inputTypes = inputType;
        else
        if (inputTypes != inputType)
            return status(SendCoinsStatusEnum::InputTypeError);

        if (inputTypes == 1)
        {
            if (nRingSize == -1)
                nRingSize = rcp.nRingSize();
            else
            if (nRingSize != rcp.nRingSize())
                return status(SendCoinsStatusEnum::RingSizeError);
        };

        if (rcp.amount() <= 0)
            return status(SendCoinsStatusEnum::InvalidAmount);

        nTotalOut += rcp.amount();
    };

    // -- first balance check
    if (inputTypes == 0)
    {
        nBalance = wallet->GetBalance();
        if (nTotalOut > nBalance)
            return status(SendCoinsStatusEnum::AmountExceedsBalance);
    } else
    {
        nBalance = wallet->GetSpectreBalance();
        if (nTotalOut > nBalance)
            return status(SendCoinsStatusEnum::SCR_AmountExceedsBalance);
    };

    {
        LOCK2(cs_main, wallet->cs_wallet);

        CWalletTx wtxNew;
        wtxNew.nVersion = ANON_TXN_VERSION;

        std::map<CScript, std::string> mapScriptNarr;
        std::map<CKeyID, CStealthAddress> mapPubStealth;
        std::vector<std::pair<CScript, int64_t> > vecSend;
        std::vector<std::pair<CScript, int64_t> > vecChange;

        // -- create outputs
        foreach(const SendCoinsRecipient &rcp, recipients)
        {
            CStealthAddress sxAddrTo;
            std::string sAddr = rcp.address().toStdString();
            if (!sxAddrTo.SetEncoded(sAddr))
                return status(rcp.txnType() == TxnTypeEnum::TXT_ANON_TO_SPEC ? SendCoinsStatusEnum::SCR_StealthAddressFailAnonToSpec :
                    SendCoinsStatusEnum::SCR_StealthAddressFail);

            int64_t nValue = rcp.amount();
            std::string sNarr = rcp.narration().toStdString();
            CScript scriptNarration; // needed to match output id of narr

            if (rcp.txnType() == TxnTypeEnum::TXT_SPEC_TO_SPEC
                || rcp.txnType() == TxnTypeEnum::TXT_ANON_TO_SPEC)
            {
                // -- out spec
                if (inputTypes == 1) {
                    // -- Check that we own the recipient address (SPECTRE to XSPEC only allowed for transformation)
                    if (!wallet->IsMine(sxAddrTo)) {
                        return status(SendCoinsStatusEnum::RecipientAddressNotOwnedSPECTREtoXSPEC);
                    }
                }

                std::string sError;

                if (!wallet->CreateStealthOutput(&sxAddrTo, nValue, sNarr, vecSend, scriptNarration, sError))
                {
                    LogPrintf("SendCoinsAnon() CreateStealthOutput failed %s.\n", sError.c_str());
                    return SendCoinsReturn(SendCoinsStatusEnum::SCR_ErrorWithMsg, 0, QString::fromStdString(sError));
                };

            } else
            {
                // -- out spectre
                if (inputTypes == 0) {
                    // -- Check that we own the recipient address (XSPEC to SPECTRE only allowed for transformation)
                    if (!wallet->IsMine(sxAddrTo)) {
                        return status(SendCoinsStatusEnum::RecipientAddressNotOwnedXSPECtoSPECTRE);
                    }
                }

                if (!wallet->CreateAnonOutputs(&sxAddrTo, nValue, sNarr, vecSend, scriptNarration, &mapPubStealth))
                {
                    LogPrintf("SendCoinsAnon() CreateAnonOutputs failed.\n");
                    return status(SendCoinsStatusEnum::SCR_Error);
                };
            };
            if (scriptNarration.size() > 0)
                mapScriptNarr[scriptNarration] = sNarr;
        };


        // -- add inputs

        if (inputTypes == 0)
        {
            // -- in XSPEC

            for (uint32_t i = 0; i < vecSend.size(); ++i)
                wtxNew.vout.push_back(CTxOut(vecSend[i].second, vecSend[i].first));

            int nChangePos = -1;

            if (!wallet->CreateTransaction(vecSend, wtxNew, nFeeRequired, nChangePos, coinControl))
            {
                if ((nTotalOut + nFeeRequired) > nBalance) // FIXME: could cause collisions in the future
                    return SendCoinsReturn(SendCoinsStatusEnum::AmountWithFeeExceedsBalance, nFeeRequired, QString());
                return status(SendCoinsStatusEnum::TransactionCreationFailed);
            };
        }
        else
        {
            // -- in SPECTRE

            std::string sError;
            if (!wallet->AddAnonInputs(nRingSize == 1 ? RING_SIG_1 : RING_SIG_2, nTotalOut, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, false, sError))
            {
                if (nFeeRequired != MAX_MONEY && (nTotalOut + nFeeRequired) > nBalance) // FIXME: could cause collisions in the future
                    return SendCoinsReturn(SendCoinsStatusEnum::SCR_AmountWithFeeExceedsSpectreBalance, nFeeRequired, QString());

                LogPrintf("SendCoinsAnon() AddAnonInputs failed %s.\n", sError.c_str());
                return SendCoinsReturn(SendCoinsStatusEnum::SCR_ErrorWithMsg, 0, QString::fromStdString(sError));
            };
        };

        for (const auto & [scriptNarration, sNarr] : mapScriptNarr)
        {
            std::string sError;
            if (!wallet->SaveNarrationOutput(wtxNew, scriptNarration, sNarr, sError))
            {
                LogPrintf("SendCoinsAnon(): %s\n",  sError.c_str());
                return SendCoinsReturn(SendCoinsStatusEnum::SCR_Error, 0, QString::fromStdString(sError));
            }
        }

//        if (!uiInterface.ThreadSafeAskFee(nFeeRequired, tr("Sending...").toStdString()))
//            return status(SendCoinsStatusEnum::Aborted);
//        if (feeApproval != nFeeRequired)
//            return SendCoinsReturn(SendCoinsStatusEnum::ApproveFee, nFeeRequired, QString());

        //return SendCoinsReturn(SCR_ErrorWithMsg, 0, QString::fromStdString(std::string("Testing error")));

        if (!wallet->CommitTransaction(wtxNew, &mapPubStealth))
        {
            LogPrintf("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.\n");
            wallet->UndoAnonTransaction(wtxNew, &mapPubStealth);
            return status(SendCoinsStatusEnum::TransactionCommitFailed);

        };

        hex = QString::fromStdString(wtxNew.GetHash().GetHex());
    } // LOCK2(cs_main, wallet->cs_wallet)


    // Add addresses / update labels that we've sent to to the address book
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        if(rcp.label().isEmpty()) // Don't add addresses without labels...
            continue;

        std::string strAddress = rcp.address().toStdString();
        std::string strLabel = rcp.label().toStdString();

        {
            LOCK(wallet->cs_wallet);

            if (rcp.addressTypeInd() == AT_Stealth)
            {
                wallet->UpdateStealthAddress(strAddress, strLabel, true);
            } else
            {
                CTxDestination dest = CBitcoinAddress(strAddress).Get();
                std::map<CTxDestination, std::string>::iterator mi = wallet->mapAddressBook.find(dest);

                // Check if we have a new address or an updated label
                if (mi == wallet->mapAddressBook.end() || mi->second != strLabel)
                {
                    wallet->SetAddressBookName(dest, strLabel);
                };
            };
        }
    };

    return SendCoinsReturn(SendCoinsStatusEnum::OK, nFeeRequired, hex);
}

OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

EncryptionStatus WalletModel::getEncryptionStatus() const
{
    if(!wallet->IsCrypted())
    {
        return EncryptionStatus::Unencrypted;
    }
    else if(wallet->IsLocked())
    {
        return EncryptionStatus::Locked;
    }
    else
    {
        return EncryptionStatus::Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(bool encrypted, const QString &passphrase)
{
    if(encrypted)
    {
        // Encrypt
        SecureString secPassphrase;
        secPassphrase.reserve(MAX_PASSPHRASE_SIZE);
        secPassphrase.assign(passphrase.toStdString().c_str());
        return wallet->EncryptWallet(secPassphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
}

bool WalletModel::lockWallet()
{
    // Lock
    return wallet->Lock();
}

bool WalletModel::unlockWallet(const QString &passPhrase, const bool fStakingOnly)
{
    // Unlock
    SecureString securePassPhrase;
    securePassPhrase.assign(passPhrase.toStdString());
    bool unlocked =  wallet->Unlock(securePassPhrase);
    if (unlocked)
        fWalletUnlockStakingOnly = fStakingOnly;
    return unlocked;
}

bool WalletModel::changePassphrase(const QString &oldPass, const QString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        SecureString secOldPass, secNewPass;
        secOldPass.reserve(MAX_PASSPHRASE_SIZE);
        secNewPass.reserve(MAX_PASSPHRASE_SIZE);
        secOldPass.assign(oldPass.toStdString().c_str());
        secNewPass.assign(newPass.toStdString().c_str());
        retval = wallet->ChangeWalletPassphrase(secOldPass, secNewPass);
    }
    return retval;
}

bool WalletModel::backupWallet(const QString &filename)
{
#ifndef ANDROID
    return BackupWallet(*wallet, filename.toLocal8Bit().data());
#else
    return ProcessWalletFile(*wallet, [filename] (const std::string& walletPath) -> bool {
        qDebug() << "backupWallet: from " << QString::fromStdString(walletPath) << " to " << filename;
        QFile walletFile = (QString::fromStdString(walletPath));
        QFile targetFile = (filename);
        if (!walletFile.open(QIODevice::ReadOnly)) {
                qDebug() << "backupWallet: Could not open wallefile";
                return false;
        }
        if (!targetFile.open(QIODevice::WriteOnly)) {
                qDebug() << "backupWallet: Could not open targetFile";
                return false;
        }
        qint64 size = targetFile.write(walletFile.readAll());
        qDebug() << "backupWallet: written bytes " << size;
        return size > -1;
    });
#endif
}


// Handlers for core signals
static void NotifyKeyStoreStatusChanged(WalletModel *walletModel, CCryptoKeyStore *wallet)
{
    LogPrintf("NotifyKeyStoreStatusChanged\n");
    QMetaObject::invokeMethod(walletModel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(WalletModel *walletModel, CWallet *wallet, const CTxDestination &address, const std::string &label, bool isMine, ChangeType status, bool fManual)
{
    if (address.type() == typeid(CStealthAddress))
    {
        CStealthAddress sxAddr = boost::get<CStealthAddress>(address);
        std::string enc = sxAddr.Encoded();
        LogPrintf("NotifyAddressBookChanged %s %s isMine=%i status=%i\n", enc.c_str(), label.c_str(), isMine, status);
        QMetaObject::invokeMethod(walletModel, "updateAddressBook", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::fromStdString(enc)),
                                  Q_ARG(QString, QString::fromStdString(label)),
                                  Q_ARG(bool, isMine),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, fManual));
    } else
    {
        LogPrintf("NotifyAddressBookChanged %s %s isMine=%i status=%i\n", CBitcoinAddress(address).ToString().c_str(), label.c_str(), isMine, status);
        QMetaObject::invokeMethod(walletModel, "updateAddressBook", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::fromStdString(CBitcoinAddress(address).ToString())),
                                  Q_ARG(QString, QString::fromStdString(label)),
                                  Q_ARG(bool, isMine),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, fManual));
    }
}

static void NotifyTransactionChanged(WalletModel *walletModel, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    LogPrintf("NotifyTransactionChanged %s status=%i\n", hash.GetHex().c_str(), status);
    QMetaObject::invokeMethod(walletModel, "updateTransaction", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(hash.GetHex())),
                              Q_ARG(int, status));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));

    connect(this, SIGNAL(encryptionStatusChanged(int)), addressTableModel, SLOT(setEncryptionStatus(int)));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));

    disconnect(this, SIGNAL(encryptionStatusChanged(int)), addressTableModel, SLOT(setEncryptionStatus(int)));
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock(WalletModel::UnlockMode mode)
{
    bool was_locked = getEncryptionStatus() == EncryptionStatus::Locked;

#ifdef ANDROID
    return UnlockContext(this, !was_locked, false); // For Unlock/Relock via qt remoteobjects
#endif

    if ((!was_locked) && fWalletUnlockStakingOnly)
    {
       lockWallet();
       was_locked = getEncryptionStatus() == EncryptionStatus::Locked;

    }
    if(was_locked)
    {
        // Request UI to unlock wallet
        emit requireUnlock(mode);
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != EncryptionStatus::Locked;

    return UnlockContext(this, valid, was_locked && !fWalletUnlockStakingOnly);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *wallet, bool valid, bool relock):
        wallet(wallet),
        valid(valid),
        relock(relock)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->lockWallet();
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

// returns a list of COutputs from COutPoints
void WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs)
{
    LOCK2(cs_main, wallet->cs_wallet);
    BOOST_FOREACH(const COutPoint& outpoint, vOutpoints)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth);
        vOutputs.push_back(out);
    }
}

// AvailableCoins + LockedCoins grouped by wallet address or stealth address (put change in one group with wallet address)
void WalletModel::listCoins(std::map<QString, std::vector<std::pair<COutput,bool>> >& mapCoins) const
{
    std::vector<COutput> vCoins;
    wallet->AvailableCoins(vCoins);

    LOCK2(cs_main, wallet->cs_wallet); // ListLockedCoins, mapWallet
    std::vector<COutPoint> vLockedCoins;

    // add locked coins
    BOOST_FOREACH(const COutPoint& outpoint, vLockedCoins)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth);
        vCoins.push_back(out);
    }

    BOOST_FOREACH(const COutput& out, vCoins)
    {
        COutput cout = out;
        bool isChangeOutput = false;
        while (wallet->IsChange(cout.tx->vout[cout.i]) && cout.tx->vin.size() > 0 && wallet->IsMine(cout.tx->vin[0]))
        {
            if (!wallet->mapWallet.count(cout.tx->vin[0].prevout.hash)) break;
            cout = COutput(&wallet->mapWallet[cout.tx->vin[0].prevout.hash], cout.tx->vin[0].prevout.n, 0);
            isChangeOutput = true;
        }

        CTxDestination address;
        if(!ExtractDestination(cout.tx->vout[cout.i].scriptPubKey, address)) continue;

        // Check if a stealth mapping address exist for output address
        if (wallet->mapAddressBook.count(address))
        {
            std::string stealthAddress = wallet->mapAddressBook.at(address);
            if (IsStealthAddressMappingLabel(stealthAddress))
            {
                mapCoins[stealthAddress.c_str()].push_back({out, isChangeOutput});
                continue;
            }
        }

        mapCoins[CBitcoinAddress(address).ToString().c_str()].push_back({out, isChangeOutput});
    }
}

QVariantMap WalletModel::signMessage(const QString &address, const QString &message)
{
    QVariantMap result;

    CBitcoinAddress addr(address.toStdString());
    if (!addr.IsValid())
    {
        result.insert("error_msg", "The entered address is invalid. Please check the address and try again.");
        return result;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        result.insert("error_msg", "The entered address does not refer to a key. Please check the address and try again.");
        return result;
    }

    WalletModel::UnlockContext ctx(requestUnlock());
    if (!ctx.isValid())
    {
        result.insert("error_msg", "Wallet unlock was cancelled.");
        return result;
    }

    CKey key;
    if (!wallet->GetKey(keyID, key))
    {
        result.insert("error_msg", "Private key for the entered address is not available.");
        return result;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << message.toStdString();

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
    {
        result.insert("error_msg" , "Message signing failed.");
        return result;
    }
    result.insert("signed_signature", QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size())));
    result.insert("error_msg", "");

    return result;
}

/* Look up stealth address for address in wallet
 */
bool WalletModel::getStealthAddress(const QString &address, CStealthAddress& stealthAddressOut) const
{
    LOCK(wallet->cs_wallet);
    std::string sAddr = address.toStdString();
    if (wallet->GetStealthAddress(sAddr, stealthAddressOut))
        return true;
    return false;
}

bool WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    return false;
}

void WalletModel::lockCoin(COutPoint& output)
{
    return;
}

void WalletModel::unlockCoin(COutPoint& output)
{
    return;
}

void WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    return;
}

int WalletModel::countLockedAnonOutputs() {
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->CountLockedAnonOutputs();
}

void WalletModel::emitBalanceChanged(qint64 balance, qint64 spectreBal, qint64 stake, qint64 spectreStake, qint64 unconfirmed, qint64 spectreUnconfirmed, qint64 immature, qint64 spectreImmature) {
    emit balanceChanged(balance, spectreBal, stake, spectreStake, unconfirmed, spectreUnconfirmed, immature, spectreImmature); }
void WalletModel::emitNumTransactionsChanged(int count) { emit numTransactionsChanged(count); }
void WalletModel::emitRequireUnlock(UnlockMode mode) { emit requireUnlock(mode); }
void WalletModel::emitError(const QString &title, const QString &message, bool modal) { emit error(title, message, modal); }
