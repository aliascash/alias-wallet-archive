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

    SendCoinsRecipient rv;

    rv.address = address;
    rv.label = label;
    rv.narration = narration;
    rv.amount = amount;

    std::string sAddr = address.toStdString();

    if (IsBIP32(sAddr.c_str()))
        rv.typeInd = 3;
    else
        rv.typeInd = address.length() > 75 ? AT_Stealth : AT_Normal;

    rv.txnTypeInd = txnType;
    rv.nRingSize = nRingSize;

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
        switch(rcp.txnTypeInd)
        {
            case TXT_SPEC_TO_SPEC:
                formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, rcp.amount), rcp.label.toHtmlEscaped(), lineBreakAddress(rcp.address)));
                inputType = 0;
                break;
            case TXT_SPEC_TO_ANON:
                formatted.append(tr("<b>%1</b> to SPECTRE, %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, rcp.amount), rcp.label.toHtmlEscaped(), lineBreakAddress(rcp.address)));
                inputType = 0;
                nAnonOutputs++;
                break;
            case TXT_ANON_TO_ANON:
                formatted.append(tr("<b>%1</b>, ring size %2 to %3 (%4)").arg(BitcoinUnits::formatWithUnitSpectre(BitcoinUnits::XSPEC, rcp.amount), QString::number(rcp.nRingSize), rcp.label.toHtmlEscaped(), lineBreakAddress(rcp.address)));
                inputType = 1;
                nAnonOutputs++;
                break;
            case TXT_ANON_TO_SPEC:
                formatted.append(tr("<b>%1</b>, ring size %2 to XSPEC, %3 (%4)").arg(BitcoinUnits::formatWithUnitSpectre(BitcoinUnits::XSPEC, rcp.amount), QString::number(rcp.nRingSize), rcp.label.toHtmlEscaped(), lineBreakAddress(rcp.address)));
                inputType = 1;
                break;
            default:
                emit sendCoinsResult(false, tr("Unknown txn type detected %1.").arg(rcp.txnTypeInd));
                return;
        }

        if (inputTypes == -1)
            inputTypes = inputType;
        else
        if (inputTypes != inputType)
        {
            emit sendCoinsResult(false, tr("Input types must match for all recipients."));
            return;
        };

        if (inputTypes == 1)
        {
            if (ringSizes == -1)
                ringSizes = rcp.nRingSize;
            else
            if (ringSizes != rcp.nRingSize)
            {
                emit sendCoinsResult(false, tr("Ring sizes must match for all recipients."));
                return;
            };

            auto [nMinRingSize, nMaxRingSize] = GetRingSizeMinMax();
            if (ringSizes < (int)nMinRingSize || ringSizes > (int)nMaxRingSize)
            {
                QString message = nMinRingSize == nMaxRingSize ? tr("Ring size must be %1.").arg(nMinRingSize) :
                                                                 tr("Ring size outside range [%1, %2].").arg(nMinRingSize).arg(nMaxRingSize);
                emit sendCoinsResult(false, message);
                return;
            };
        };
    };

// Confirmation Question
    bool conversionTx = recipients.size() == 1 && (recipients[0].txnTypeInd == TXT_SPEC_TO_ANON || recipients[0].txnTypeInd == TXT_ANON_TO_SPEC);
    QMessageBox::StandardButton retval = QMessageBox::question(window, tr("Confirm send coins"),
       (conversionTx ? tr("Are you sure you want to convert %1?") : tr("Are you sure you want to send %1?"))
       .arg(formatted.join(tr(" and "))),
       QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) {
        emit sendCoinsResult(false, "");
        return;
    }

    WalletModel::SendCoinsReturn sendstatus;

    if (fUseCoinControl)
    {
        if (sChangeAddr.length() > 0)
        {
            CBitcoinAddress addrChange = CBitcoinAddress(sChangeAddr.toStdString());
            if (!addrChange.IsValid())
            {
               emit sendCoinsResult(false, tr("The change address is not valid, please recheck."));
               return;
            };
            CoinControlDialog::coinControl->destChange = addrChange.Get();
        } else
            CoinControlDialog::coinControl->destChange = CNoDestination();
    };

    // TODO WalletModel::UnlockContext ctx(walletModel->requestUnlock());

    // Unlock wallet was cancelled
//    if(!ctx.isValid()) {
//        emit sendCoinsResult(false, tr("Payment not send because wallet is locked."));
//        return;
//    }

//    if (inputTypes == 1 || nAnonOutputs > 0)
//        sendstatus = walletModel->sendCoinsAnon(recipients, fUseCoinControl ? CoinControlDialog::coinControl : NULL);
//    else
//        sendstatus = walletModel->sendCoins(recipients, fUseCoinControl ? CoinControlDialog::coinControl : NULL);

    switch(sendstatus.status)
    {
        case WalletModel::InvalidAddress:
            emit sendCoinsResult(false, tr("The recipient address is not valid, please recheck."));
            return;
        case WalletModel::StealthAddressOnlyAllowedForSPECTRE:
            emit sendCoinsResult(false, tr("Only SPECTRE from your Private balance can be send to a stealth address."));
            return;
        case WalletModel::RecipientAddressNotOwnedXSPECtoSPECTRE:
            emit sendCoinsResult(false, tr("Transfer from Public to Private (XSPEC to SPECTRE) is only allowed within your account."));
            return;
        case WalletModel::RecipientAddressNotOwnedSPECTREtoXSPEC:
            emit sendCoinsResult(false, tr("Transfer from Private to Public (SPECTRE to XSPEC) is only allowed within your account."));
            return;
        case WalletModel::InvalidAmount:
            emit sendCoinsResult(false, tr("The amount to pay must be larger than 0."));
            return;
        case WalletModel::AmountExceedsBalance:
            emit sendCoinsResult(false, tr("The amount exceeds your balance."));
            return;
        case WalletModel::AmountWithFeeExceedsBalance:
            emit sendCoinsResult(false, tr("The total exceeds your balance when the %1 transaction fee is included."));
            return;
        case WalletModel::DuplicateAddress:
            emit sendCoinsResult(false, tr("Duplicate address found, can only send to each address once per send operation."));
            return;
        case WalletModel::TransactionCreationFailed:
            emit sendCoinsResult(false, tr("Error: Transaction creation failed."));
            return;
        case WalletModel::TransactionCommitFailed:
            emit sendCoinsResult(false, tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."));
            return;
        case WalletModel::NarrationTooLong:
            emit sendCoinsResult(false, tr("Error: Narration is too long."));
            return;
        case WalletModel::RingSizeError:
            emit sendCoinsResult(false, tr("Error: Ring Size Error."));
            return;
        case WalletModel::InputTypeError:
            emit sendCoinsResult(false, tr("Error: Input Type Error."));
            return;
        case WalletModel::SCR_NeedFullMode:
            emit sendCoinsResult(false, tr("Error: Must be in full mode to send anon."));
            return;
        case WalletModel::SCR_StealthAddressFail:
            emit sendCoinsResult(false, tr("Error: Invalid Stealth Address."));
            return;
        case WalletModel::SCR_StealthAddressFailAnonToSpec:
            emit sendCoinsResult(false, tr("Error: Invalid Stealth Address. SPECTRE to XSPEC conversion requires a stealth address."));
            return;
		case WalletModel::SCR_AmountExceedsBalance:
            emit sendCoinsResult(false, tr("The amount exceeds your SPECTRE balance."));
			return;
        case WalletModel::SCR_AmountWithFeeExceedsSpectreBalance:
            emit sendCoinsResult(false, tr("The total exceeds your SPECTRE balance when the %1 transaction fee is included.").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, sendstatus.fee)));
            return;
        case WalletModel::SCR_Error:
            emit sendCoinsResult(false, tr("Error generating transaction."));
            return;
        case WalletModel::SCR_ErrorWithMsg:
            emit sendCoinsResult(false, tr("Error generating transaction: %1").arg(sendstatus.hex));
            return;

        case WalletModel::Aborted: // User aborted, nothing to do
            emit sendCoinsResult(false, "");
            return;
        case WalletModel::OK:
            //accept();
            CoinControlDialog::coinControl->UnSelectAll();
            CoinControlDialog::payAmounts.clear();
            // TODO CoinControlDialog::updateLabels(walletModel, 0, this);
            recipients.clear();
            break;
    }

    emit sendCoinsResult(true, tr("Transaction successfully created."));
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

QJsonValue SpectreClientBridge::userAction(QJsonValue action)
{
    QString key = action.toArray().at(0).toString();
    if (key == "") {
        key = action.toObject().keys().at(0);
    }
#ifndef ANDROID // TODO emit signals to QT UI
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
#endif
    if(key == "clearRecipients")
        clearRecipients();

    return QJsonValue();
}
