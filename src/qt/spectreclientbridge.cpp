// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

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
#include "aliasbridgestrings.h"

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
#include <QMessageBox>

#ifdef ANDROID
#include <QAndroidIntent>
#include <QtAndroidExtras>
#endif


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

void SpectreClientBridge::scanQRCode()
{
#ifdef ANDROID
    QtAndroid::androidActivity().callMethod<void>("scanQRCode", "()V");
#endif
}

// Transactions
void SpectreClientBridge::addRecipient(QString address, QString label, QString narration, qint64 amount, int txnType)
{
// TODO Don't validate address atm, because UI does not properly handle addRecipientResult(false), address will be validated in sendCoins
//    QRemoteObjectPendingReply<bool> validateAddressPendingReply = window->walletModel->validateAddress(address);
//    if (!validateAddressPendingReply.waitForFinished() || !validateAddressPendingReply.returnValue()) {
//        emit addRecipientResult(false);
//        return;
//    }

    std::string sAddr = address.toStdString();

    SendCoinsRecipient rv(address, label, narration,
                               IsBIP32(sAddr.c_str()) ? 3 : address.length() > 75 ? AT_Stealth : AT_Normal,
                               amount, TxnTypeEnum::TxnType(txnType), GetRingSizeMinMax().first);
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
                formatted.append(tr("<b>%1</b> from your public balance to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::ALIAS, rcp.amount()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 0;
                break;
            case TxnTypeEnum::TXT_SPEC_TO_ANON:
                formatted.append(tr("<b>%1</b> from public to private, using address %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::ALIAS, rcp.amount()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 0;
                nAnonOutputs++;
                break;
            case TxnTypeEnum::TXT_ANON_TO_ANON:
                formatted.append(tr("<b>%1</b> from your private balance, ring size %2, to %3 (%4)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::ALIAS, rcp.amount()), QString::number(rcp.nRingSize()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 1;
                nAnonOutputs++;
                break;
            case TxnTypeEnum::TXT_ANON_TO_SPEC:
                formatted.append(tr("<b>%1</b> from private to public, ring size %2, using address %3 (%4)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::ALIAS, rcp.amount()), QString::number(rcp.nRingSize()), rcp.label().toHtmlEscaped(), lineBreakAddress(rcp.address())));
                inputType = 1;
                break;
            default:
                return abortSendCoins(tr("Unknown txn type detected %1.").arg(rcp.txnType()));
        }

        if (inputTypes == -1)
            inputTypes = inputType;
        else
        if (inputTypes != inputType)
            return abortSendCoins(tr("Input types must match for all recipients."));

        if (inputTypes == 1)
        {
            if (ringSizes == -1)
                ringSizes = rcp.nRingSize();
            else
            if (ringSizes != rcp.nRingSize())
                return abortSendCoins(tr("Ring sizes must match for all recipients."));

            auto [nMinRingSize, nMaxRingSize] = GetRingSizeMinMax();
            if (ringSizes < (int)nMinRingSize || ringSizes > (int)nMaxRingSize)
            {
                QString message = nMinRingSize == nMaxRingSize ? tr("Ring size must be %1.").arg(nMinRingSize) :
                                                                 tr("Ring size outside range [%1, %2].").arg(nMinRingSize).arg(nMaxRingSize);
                return abortSendCoins(message);
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
        return abortSendCoins(tr("Payment not send because wallet is locked."));
    }

    QProgressDialog* pProgressDlg = window->showProgressDlg("Creating transaction...");

    QList<OutPoint> coins; // TODO fUseCoinControl ? CoinControlDialog::coinControl
    QRemoteObjectPendingReply<SendCoinsReturn> sendResultPendingReply;
    if (inputTypes == 1 || nAnonOutputs > 0)
        sendResultPendingReply = window->walletModel->sendCoinsAnon(0, recipients, coins);
    else
        sendResultPendingReply = window->walletModel->sendCoins(0, recipients, coins);

    if (!sendResultPendingReply.waitForFinished())
         return abortSendCoins(tr("Core not responding."));

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
            if (!sendResultPendingReply.waitForFinished())
                return abortSendCoins(tr("Core not responding."));
            sendResult = sendResultPendingReply.returnValue();
            sendCoinsStatus = sendResult.status();
        }
    }

    switch(sendCoinsStatus)
    {
        case SendCoinsStatusEnum::InvalidAddress:
            return abortSendCoins(tr("The recipient address is not valid, please recheck."));
        case SendCoinsStatusEnum::StealthAddressOnlyAllowedForSPECTRE:
            return abortSendCoins(tr("Only ALIAS from your Private balance can be send to a private address."));
        case SendCoinsStatusEnum::RecipientAddressNotOwnedXSPECtoSPECTRE:
            return abortSendCoins(tr("Transfer from Public to Private is only allowed within your account."));
        case SendCoinsStatusEnum::RecipientAddressNotOwnedSPECTREtoXSPEC:
            return abortSendCoins(tr("Transfer from Private to Public is only allowed within your account."));
        case SendCoinsStatusEnum::InvalidAmount:
            return abortSendCoins(tr("The amount to pay must be larger than 0."));
        case SendCoinsStatusEnum::AmountExceedsBalance:
            return abortSendCoins(tr("The amount exceeds your balance."));
        case SendCoinsStatusEnum::AmountWithFeeExceedsBalance:
            return abortSendCoins(tr("The total exceeds your balance when the %1 transaction fee is included."));
        case SendCoinsStatusEnum::DuplicateAddress:
            return abortSendCoins(tr("Duplicate address found, can only send to each address once per send operation."));
        case SendCoinsStatusEnum::TransactionCreationFailed:
            return abortSendCoins(tr("Error: Transaction creation failed."));
        case SendCoinsStatusEnum::TransactionCommitFailed:
            return abortSendCoins(tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."));
        case SendCoinsStatusEnum::NarrationTooLong:
            return abortSendCoins(tr("Error: Note is too long."));
        case SendCoinsStatusEnum::RingSizeError:
            return abortSendCoins(tr("Error: Ring Size Error."));
        case SendCoinsStatusEnum::InputTypeError:
            return abortSendCoins(tr("Error: Input Type Error."));
        case SendCoinsStatusEnum::SCR_NeedFullMode:
            return abortSendCoins(tr("Error: Must be in full mode to send anon."));
        case SendCoinsStatusEnum::SCR_StealthAddressFail:
            return abortSendCoins(tr("Error: Invalid Private Address."));
        case SendCoinsStatusEnum::SCR_StealthAddressFailAnonToSpec:
            return abortSendCoins(tr("Error: Invalid Private Address. Private to public conversion requires a stealth address."));
        case SendCoinsStatusEnum::SCR_AmountExceedsBalance:
            return abortSendCoins(tr("The amount exceeds your ALIAS balance."));
        case SendCoinsStatusEnum::SCR_AmountWithFeeExceedsSpectreBalance:
            return abortSendCoins(tr("The total exceeds your private ALIAS balance when the %1 transaction fee is included.").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::ALIAS, sendResult.fee())));
        case SendCoinsStatusEnum::SCR_Error:
            return abortSendCoins(tr("Error generating transaction."));
        case SendCoinsStatusEnum::SCR_ErrorWithMsg:
            return abortSendCoins(tr("Error generating transaction: %1").arg(sendResult.hex()));
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
                            tr("Transaction successfully created. Fee payed %1").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::ALIAS, sendResult.fee())));
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
    if(key == "resetBlockainClicked")
        window->resetBlockchain();
    if(key == "rewindBlockchainClicked")
        window->rewindBlockchain();
    if(key == "debugClicked")
    {
//  TODO
//        window->rpcConsole->show();
//        window->rpcConsole->activateWindow();
//        window->rpcConsole->raise();
    }
    if(key == "clearRecipients")
        clearRecipients();
}

QVariantMap SpectreClientBridge::signMessage(QString address, QString message)
{
    QVariantMap result;
    SpectreGUI::UnlockContext ctx(window->requestUnlock());
    if (!ctx.isValid())
    {
        result.insert("error_msg", "Wallet unlock was cancelled.");
        return result;
    }

    QRemoteObjectPendingReply<QVariantMap> reply = window->walletModel->signMessage(address, message);
    if (reply.waitForFinished())
        result = reply.returnValue();
    else {
        result.insert("error_msg", "Core not responding.");
    }
    return result;
}

void SpectreClientBridge::newAddress(QString addressLabel, int addressType, QString address, bool send)
{
    if (!send)
    {
        // Unlock wallet
        SpectreGUI::UnlockContext ctx(window->requestUnlock());
        // Unlock wallet was cancelled
        if(!ctx.isValid()) {
            emit newAddressResult(false, tr("Wallet locked."), "", send);
        }
        // Generate a new address to associate with given label
        QRemoteObjectPendingReply<NewAddressResult> reply = window->addressModel->newReceiveAddress(addressType, addressLabel);
        if (reply.waitForFinished())
            emit newAddressResult(reply.returnValue().success(), reply.returnValue().errorMsg(), reply.returnValue().address(), send);
        else
            emit newAddressResult(false, tr("Core not responding."), "", send);
    }
    else {
        // Generate a new address to associate with given label
        QRemoteObjectPendingReply<NewAddressResult> reply = window->addressModel->newSendAddress(addressType, addressLabel, address);
        if (reply.waitForFinished())
            emit newAddressResult(reply.returnValue().success(), reply.returnValue().errorMsg(), reply.returnValue().address(), send);
        else
            emit newAddressResult(false, tr("Core not responding."), "", send);
    }
}

void SpectreClientBridge::translateHtmlString(QString string)
{
    std::string result = QCoreApplication::translate("alias-bridge", qPrintable(string)).toStdString();
    LogPrintf("translateHtmlString: '%s' -> '%s'\n", string.toStdString(), result);
    emit updateElement(string, QString::fromStdString(result));
}
