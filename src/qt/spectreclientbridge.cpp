// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "spectreclientbridge.h"

#include "spectregui.h"
#include "guiutil.h"

#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#endif

#include "base58.h"
#include "bitcoinunits.h"
#include "coincontrol.h"
#include "coincontroldialog.h"
#include "ringsig.h"

#include "askpassphrasedialog.h"

#include "extkey.h"

#include "addresstablemodel.h" // Need enum EAddressType
#include "bridgetranslations.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QJsonObject>

#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>
#include <QDir>
#include <QtGui/qtextdocument.h>
#include <QDebug>
#include <list>
#include <QProgressDialog>


SpectreClientBridge::SpectreClientBridge(SpectreGUI *window, QWebChannel *webChannel, QObject *parent) :
    QObject         (parent),
    window          (window),
    webChannel      (webChannel)
{
    // register models to be exposed to JavaScript
    webChannel->registerObject(QStringLiteral("clientBridge"), this);
    window->clientBridge = this;
}

SpectreClientBridge::~SpectreClientBridge()
{
}

void SpectreClientBridge::copy(QString text)
{
    QApplication::clipboard()->setText(text);
}

void SpectreClientBridge::paste()
{
    emitPaste(QApplication::clipboard()->text());
}

void SpectreClientBridge::urlClicked(const QString link)
{
    emit window->urlClicked(QUrl(link));
}

// Transactions
void SpectreClientBridge::addRecipient(QString address, QString label, QString narration, qint64 amount, int txnType, int nRingSize)
{
// TODO   if (!walletModel->validateAddress(address)) {
//        emit addRecipientResult(false);
//        return;
//    }

    std::string sAddr = address.toStdString();

    SendCoinsRecipient rv(address, label, narration,
                               IsBIP32(sAddr.c_str()) ? 3 : address.length() > 75 ? AT_Stealth : AT_Normal,
                               amount, TxnTypeEnum::TxnType(txnType), nRingSize);
    recipients.append(rv);

    emit addRecipientResult(true);
}

void SpectreClientBridge::clearRecipients()
{
    recipients.clear();
}

QString lineBreakAddress(QString address)
{
    if (address.length() > 50)
        return address.left(address.length() / 2) + "<br>" + address.mid(address.length() / 2);
    else
        return address;
}

void SpectreClientBridge::abortSendCoins(QString message)
{
    QMessageBox::warning(window, tr("Send Coins"), message,
                  QMessageBox::Ok, QMessageBox::Ok);
    emit sendCoinsResult(false);
}


void SpectreClientBridge::sendCoins(bool fUseCoinControl, QString sChangeAddr)
{
    int inputTypes = -1;
    int nAnonOutputs = 0;
    int ringSizes = -1;
    // Format confirmation message
    QStringList formatted;
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        int inputType; // 0 XSPEC, 1 Spectre
        switch(rcp.txnType())
        {
            case TxnTypeEnum::TXT_SPEC_TO_SPEC:
                formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, rcp.amount()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 0;
                break;
            case TxnTypeEnum::TXT_SPEC_TO_ANON:
                formatted.append(tr("<b>%1</b> to SPECTRE, %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, rcp.amount()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 0;
                nAnonOutputs++;
                break;
            case TxnTypeEnum::TXT_ANON_TO_ANON:
                formatted.append(tr("<b>%1</b>, ring size %2 to %3 (%4)").arg(BitcoinUnits::formatWithUnitSpectre(BitcoinUnits::XSPEC, rcp.amount()), QString::number(rcp.nRingSize()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 1;
                nAnonOutputs++;
                break;
            case TxnTypeEnum::TXT_ANON_TO_SPEC:
                formatted.append(tr("<b>%1</b>, ring size %2 to XSPEC, %3 (%4)").arg(BitcoinUnits::formatWithUnitSpectre(BitcoinUnits::XSPEC, rcp.amount()), QString::number(rcp.nRingSize()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 1;
                break;
            default:
                abortSendCoins(tr("Unknown txn type detected %1.").arg(rcp.txnType()));
                return;
        }

        if (inputTypes == -1)
            inputTypes = inputType;
        else
        if (inputTypes != inputType)
        {
            abortSendCoins(tr("Input types must match for all recipients."));
            return;
        };

        if (inputTypes == 1)
        {
            if (ringSizes == -1)
                ringSizes = rcp.nRingSize();
            else
            if (ringSizes != rcp.nRingSize())
            {
                abortSendCoins(tr("Ring sizes must match for all recipients."));
                return;
            };

            auto [nMinRingSize, nMaxRingSize] = GetRingSizeMinMax();
            if (ringSizes < (int)nMinRingSize || ringSizes > (int)nMaxRingSize)
            {
                QString message = nMinRingSize == nMaxRingSize ? tr("Ring size must be %1.").arg(nMinRingSize) :
                                                                 tr("Ring size outside range [%1, %2].").arg(nMinRingSize).arg(nMaxRingSize);
                abortSendCoins(message);
                return;
            };
        };
    };

// Confirmation Question
    bool conversionTx = recipients.size() == 1 && (recipients[0].txnType() == TxnTypeEnum::TXT_SPEC_TO_ANON || recipients[0].txnType() == TxnTypeEnum::TXT_ANON_TO_SPEC);
    QMessageBox::StandardButton retval = QMessageBox::question(window, tr("Confirm send coins"),
       (conversionTx ? tr("Are you sure you want to convert %1?") : tr("Are you sure you want to send %1?"))
       .arg(formatted.join(tr(" and "))),
       QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) {
        emit sendCoinsResult(false);
        return;
    }

    SendCoinsReturn sendResult;

    if (fUseCoinControl)
    {
        if (sChangeAddr.length() > 0)
        {
            CBitcoinAddress addrChange = CBitcoinAddress(sChangeAddr.toStdString());
            if (!addrChange.IsValid())
            {
               abortSendCoins(tr("The change address is not valid, please recheck."));
               return;
            };
            CoinControlDialog::coinControl->destChange = addrChange.Get();
        } else
            CoinControlDialog::coinControl->destChange = CNoDestination();
    };

    // Unlock wallet
    SpectreGUI::UnlockContext ctx(window->requestUnlock());
    // Unlock wallet was cancelled
    if(!ctx.isValid()) {
        abortSendCoins(tr("Payment not send because wallet is locked."));
        return;
    }

    QProgressDialog* pProgressDlg = window->showProgressDlg("Creating transaction...");

    QList<OutPoint> coins; // TODO fUseCoinControl ? CoinControlDialog::coinControl
    QRemoteObjectPendingReply<SendCoinsReturn> sendResultPendingReply;
    if (inputTypes == 1 || nAnonOutputs > 0)
        sendResultPendingReply = window->walletModel->sendCoinsAnon(0, recipients, coins);
    else
        sendResultPendingReply = window->walletModel->sendCoins(0, recipients, coins);

    sendResultPendingReply.waitForFinished(-1);
    sendResult = sendResultPendingReply.returnValue();
    SendCoinsStatusEnum::SendCoinsStatus sendCoinsStatus = sendResult.status();

    window->closeProgressDlg(pProgressDlg);

    if (sendCoinsStatus == SendCoinsStatusEnum::ApproveFee)
    {
        bool payFee = false;;
        window->askFee(sendResult.fee(), &payFee);
        if (!payFee)
           sendCoinsStatus = SendCoinsStatusEnum::Aborted;
        else {
            if (inputTypes == 1 || nAnonOutputs > 0)
                sendResultPendingReply = window->walletModel->sendCoinsAnon(sendResult.fee(), recipients, coins);
            else
                sendResultPendingReply = window->walletModel->sendCoins(sendResult.fee(), recipients, coins);
            sendResultPendingReply.waitForFinished(-1);
            sendResult = sendResultPendingReply.returnValue();
            sendCoinsStatus = sendResult.status();
        }
    }

    switch(sendCoinsStatus)
    {
        case SendCoinsStatusEnum::InvalidAddress:
            abortSendCoins(tr("The recipient address is not valid, please recheck."));
            return;
        case SendCoinsStatusEnum::StealthAddressOnlyAllowedForSPECTRE:
            abortSendCoins(tr("Only SPECTRE from your Private balance can be send to a stealth address."));
            return;
        case SendCoinsStatusEnum::RecipientAddressNotOwnedXSPECtoSPECTRE:
            abortSendCoins(tr("Transfer from Public to Private (XSPEC to SPECTRE) is only allowed within your account."));
            return;
        case SendCoinsStatusEnum::RecipientAddressNotOwnedSPECTREtoXSPEC:
            abortSendCoins(tr("Transfer from Private to Public (SPECTRE to XSPEC) is only allowed within your account."));
            return;
        case SendCoinsStatusEnum::InvalidAmount:
            abortSendCoins(tr("The amount to pay must be larger than 0."));
            return;
        case SendCoinsStatusEnum::AmountExceedsBalance:
            abortSendCoins(tr("The amount exceeds your balance."));
            return;
        case SendCoinsStatusEnum::AmountWithFeeExceedsBalance:
            abortSendCoins(tr("The total exceeds your balance when the %1 transaction fee is included."));
            return;
        case SendCoinsStatusEnum::DuplicateAddress:
            abortSendCoins(tr("Duplicate address found, can only send to each address once per send operation."));
            return;
        case SendCoinsStatusEnum::TransactionCreationFailed:
            abortSendCoins(tr("Error: Transaction creation failed."));
            return;
        case SendCoinsStatusEnum::TransactionCommitFailed:
            abortSendCoins(tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."));
            return;
        case SendCoinsStatusEnum::NarrationTooLong:
            abortSendCoins(tr("Error: Narration is too long."));
            return;
        case SendCoinsStatusEnum::RingSizeError:
            abortSendCoins(tr("Error: Ring Size Error."));
            return;
        case SendCoinsStatusEnum::InputTypeError:
            abortSendCoins(tr("Error: Input Type Error."));
            return;
        case SendCoinsStatusEnum::SCR_NeedFullMode:
            abortSendCoins(tr("Error: Must be in full mode to send anon."));
            return;
        case SendCoinsStatusEnum::SCR_StealthAddressFail:
            abortSendCoins(tr("Error: Invalid Stealth Address."));
            return;
        case SendCoinsStatusEnum::SCR_StealthAddressFailAnonToSpec:
            abortSendCoins(tr("Error: Invalid Stealth Address. SPECTRE to XSPEC conversion requires a stealth address."));
            return;
        case SendCoinsStatusEnum::SCR_AmountExceedsBalance:
            abortSendCoins(tr("The amount exceeds your SPECTRE balance."));
			return;
        case SendCoinsStatusEnum::SCR_AmountWithFeeExceedsSpectreBalance:
            abortSendCoins(tr("The total exceeds your SPECTRE balance when the %1 transaction fee is included.").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, sendResult.fee())));
            return;
        case SendCoinsStatusEnum::SCR_Error:
            abortSendCoins(tr("Error generating transaction."));
            return;
        case SendCoinsStatusEnum::SCR_ErrorWithMsg:
            abortSendCoins(tr("Error generating transaction: %1").arg(sendResult.hex()));
            return;

        case SendCoinsStatusEnum::ApproveFee:
        case SendCoinsStatusEnum::Aborted: // User aborted, nothing to do
            emit sendCoinsResult(false);
            return;
        case SendCoinsStatusEnum::OK:
            //accept();
            CoinControlDialog::coinControl->UnSelectAll();
            CoinControlDialog::payAmounts.clear();
            // TODO CoinControlDialog::updateLabels(walletModel, 0, this);
            QMessageBox::information(window, tr("Send Coins"),
                            tr("Transaction successfully created. Fee payed %1").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, sendResult.fee())));
            recipients.clear();
            break;
    }

    emit sendCoinsResult(true);
            return;
}

void SpectreClientBridge::openCoinControl()
{
// TODO emit signal to openCoinControl dialog
//    if (!window || !walletModel)
//        return;

//    CoinControlDialog dlg;
//    dlg.setModel(walletModel);
//    dlg.exec();

//    CoinControlDialog::updateLabels(walletModel, 0, this);
}

void SpectreClientBridge::updateCoinControlAmount(qint64 amount)
{
    CoinControlDialog::payAmounts.clear();
    CoinControlDialog::payAmounts.append(amount);
    // TODO CoinControlDialog::updateLabels(walletModel, 0, this);
}

void SpectreClientBridge::updateCoinControlLabels(unsigned int &quantity, qint64 &amount, qint64 &fee, qint64 &afterfee, unsigned int &bytes, QString &priority, QString low, qint64 &change)
{
    emitCoinControlUpdate(quantity, amount, fee, afterfee, bytes, priority, low, change);
}

void SpectreClientBridge::userAction(QJsonValue action)
{
    QString key = action.toArray().at(0).toString();
    if (key == "") {
        key = action.toObject().keys().at(0);
    }
    if(key == "backupWallet")
        window->backupWallet();
    if(key == "close")
        window->close();
    if(key == "encryptWallet")
        window->encryptWallet(true);
    if(key == "changePassphrase")
        window->changePassphrase();
    if(key == "toggleLock")
        window->toggleLock();
    if(key == "aboutClicked")
        window->aboutClicked();
    if(key == "aboutQtClicked")
        window->aboutQtAction->trigger();
    if(key == "debugClicked")
    {
        window->rpcConsole->show();
        window->rpcConsole->activateWindow();
        window->rpcConsole->raise();
    }
    if(key == "clearRecipients")
        clearRecipients();
}
