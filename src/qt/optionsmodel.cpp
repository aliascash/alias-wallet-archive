// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "optionsmodel.h"
#include "bitcoinunits.h"
#include <QSettings>

#include "init.h"
#include "walletdb.h"
#include "guiutil.h"
#include "ringsig.h"

OptionsModel::OptionsModel(QObject *parent) :
    OptionsModelRemoteSimpleSource(parent)
{
    Init();
}

void OptionsModel::Init()
{
    QSettings settings;

    // These are Qt-only settings:
    //setDisplayUnit(settings.value("nDisplayUnit", BitcoinUnits::XSPEC).toInt());
    setDisplayUnit(BitcoinUnits::XSPEC);
    setDisplayAddresses(settings.value("bDisplayAddresses", false).toBool());
    setMinimizeToTray(settings.value("fMinimizeToTray", false).toBool());
    setMinimizeOnClose(settings.value("fMinimizeOnClose", false).toBool());
    nTransactionFee = settings.value("nTransactionFee").toLongLong() >= nMinTxFee ? settings.value("nTransactionFee").toLongLong() : nMinTxFee;
    nReserveBalance = settings.value("nReserveBalance").toLongLong();
    setLanguage(settings.value("language", "").toString());
    setRowsPerPage(settings.value("nRowsPerPage", 20).toInt());
    setNotifications(settings.value("notifications", "*").toStringList());
    setVisibleTransactions(settings.value("visibleTransactions", "*").toStringList());
    setAutoRingSize(settings.value("fAutoRingSize", false).toBool());
    setAutoRedeemSpectre(settings.value("fAutoRedeemSpectre", false).toBool());
    setMinRingSize(settings.value("nMinRingSize", MIN_RING_SIZE).toInt());
    setMaxRingSize(settings.value("nMaxRingSize", MIN_RING_SIZE).toInt());

    // These are shared with core Bitcoin; we want
    // command-line options to override the GUI settings:
    if (settings.contains("detachDB"))
        SoftSetBoolArg("-detachdb", settings.value("detachDB").toBool());
    if (!language().isEmpty())
        SoftSetArg("-lang", language().toStdString());
    if (settings.contains("fStaking"))
    {
        setStaking(settings.value("fStaking").toBool());
        SoftSetBoolArg("-staking", staking());
        fIsStakingEnabled = staking();
    }
    if (settings.contains("nMinStakeInterval"))
        SoftSetArg("-minstakeinterval", settings.value("nMinStakeInterval").toString().toStdString());
    if (settings.contains("nStakingDonation"))
        SoftSetArg("-stakingdonation", settings.value("nStakingDonation").toString().toStdString());
    if (settings.contains("fThinMode"))
        SoftSetBoolArg("-thinmode", settings.value("fThinMode").toBool());
    if (settings.contains("fThinFullIndex"))
        SoftSetBoolArg("-thinfullindex", settings.value("fThinFullIndex").toBool());
    if (settings.contains("nThinIndexWindow"))
        SoftSetArg("-thinindexmax", settings.value("nThinIndexWindow").toString().toStdString());
}

int OptionsModel::rowCount() const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const int row) const
{
    QSettings settings;
    switch(row)
    {
    case StartAtStartup:
        return GUIUtil::GetStartOnSystemStartup();
    case MinimizeToTray:
        return minimizeToTray();
    case MinimizeOnClose:
        return minimizeOnClose();
    case Fee:
        return (qint64) nTransactionFee;
    case ReserveBalance:
        return (qint64) nReserveBalance;
    case DisplayUnit:
        return displayUnit();
    case DisplayAddresses:
        return displayAddresses();
    case DetachDatabases:
        return bitdb.GetDetach();
    case Language:
        return settings.value("language", "");
    case RowsPerPage:
        return rowsPerPage();
    case AutoRingSize:
        return autoRingSize();
    case AutoRedeemSpectre:
        return autoRedeemSpectre();
    case MinRingSize:
        return minRingSize();
    case MaxRingSize:
        return maxRingSize();
    case Staking:
        return settings.value("fStaking", GetBoolArg("-staking", true)).toBool();
    case StakingDonation:
        if (nStakingDonation < 0) {
            nStakingDonation = 0;
        }
        return nStakingDonation;
    case MinStakeInterval:
        return nMinStakeInterval;
      case ThinMode:
        return settings.value("fThinMode",      GetBoolArg("-thinmode",      false)).toBool();
    case ThinFullIndex:
        return settings.value("fThinFullIndex", GetBoolArg("-thinfullindex", false)).toBool();
    case ThinIndexWindow:
        return settings.value("ThinIndexWindow", (qint64) GetArg("-thinindexwindow", 4096)).toInt();
    case Notifications:
        return notifications();
    case VisibleTransactions:
        return visibleTransactions();
    }
    return QVariant();
}

QString OptionsModel::optionIDName(int row)
{
    switch(row)
    {
    case Fee: return "Fee";
    case ReserveBalance: return "ReserveBalance";
    case StartAtStartup: return "StartAtStartup";
    case DetachDatabases: return "DetachDatabases";
    case Staking: return "Staking";
    case StakingDonation: return "StakingDonation";
    case MinStakeInterval: return "MinStakeInterval";
    case ThinMode: return "ThinMode";
    case ThinFullIndex: return "ThinFullIndex";
    case ThinIndexWindow: return "ThinIndexWindow";
    case AutoRingSize: return "AutoRingSize";
    case AutoRedeemSpectre: return "AutoRedeemSpectre";
    case MinRingSize: return "MinRingSize";
    case MaxRingSize: return "MaxRingSize";
    case MinimizeToTray: return "MinimizeToTray";
    case MinimizeOnClose: return "MinimizeOnClose";
    case Language: return "Language";
    case DisplayUnit: return "DisplayUnit";
    case DisplayAddresses: return "DisplayAddresses";
    case RowsPerPage: return "RowsPerPage";
    case Notifications: return "Notifications";
    case VisibleTransactions: return "VisibleTransactions";
    }

    return "";
}

int OptionsModel::optionNameID(QString name)
{
    for(int i=0;i<OptionIDRowCount;i++)
        if(optionIDName(i) == name)
            return i;

    return -1;
}

bool OptionsModel::setData(const int row, const QVariant & value)
{
    bool successful = true; /* set to false on parse error */
    QSettings settings;
    switch(row)
    {
    case StartAtStartup:
        successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
        break;
    case MinimizeToTray:
        setMinimizeToTray(value.toBool());
        settings.setValue("fMinimizeToTray", minimizeToTray());
        break;
    case MinimizeOnClose:
        setMinimizeOnClose(value.toBool());
        settings.setValue("fMinimizeOnClose", minimizeOnClose());
        break;
    case Fee:  
        nTransactionFee = value.toLongLong() < nMinTxFee ? nMinTxFee : value.toLongLong();
        settings.setValue("nTransactionFee", (qint64) nTransactionFee);
        emit transactionFeeChanged(nTransactionFee);
        break;
    case ReserveBalance:
        nReserveBalance = value.toLongLong();
        settings.setValue("nReserveBalance", (qint64) nReserveBalance);
        emit reserveBalanceChanged(nReserveBalance);
        break;
    case DisplayUnit:
        //setDisplayUnit(value.toInt());
        //settings.setValue("nDisplayUnit", displayUnit());
        break;
    case DisplayAddresses:
        setDisplayAddresses(value.toBool());
        settings.setValue("bDisplayAddresses", displayAddresses());
        Q_EMIT displayUnitChanged(displayUnit());
        break;
    case DetachDatabases: {
        bool fDetachDB = value.toBool();
        bitdb.SetDetach(fDetachDB);
        settings.setValue("detachDB", fDetachDB);
        }
        break;
    case Language:
        settings.setValue("language", value);
        break;
    case RowsPerPage: {
        setRowsPerPage(value.toInt());
        settings.setValue("nRowsPerPage", rowsPerPage());
        }
        break;
    case Notifications: {
        setNotifications(value.toStringList());
        settings.setValue("notifications", notifications());
        }
        break;
    case VisibleTransactions: {
        setVisibleTransactions(value.toStringList());
        settings.setValue("visibleTransactions", visibleTransactions());
        }
        break;
    case AutoRingSize: {
        setAutoRingSize(value.toBool());
        settings.setValue("fAutoRingSize", autoRingSize());
        }
        break;
    case AutoRedeemSpectre: {
        setAutoRedeemSpectre(value.toBool());
        settings.setValue("fAutoRedeemSpectre", autoRedeemSpectre());
        }
        break;
    case MinRingSize: {
        setMinRingSize(value.toInt());
        settings.setValue("nMinRingSize", minRingSize());
        }
        break;
    case MaxRingSize: {
        setMaxRingSize(value.toInt());
        settings.setValue("nMaxRingSize", maxRingSize());
        }
        break;
    case Staking:
        setStaking(value.toBool());
        settings.setValue("fStaking", staking());
        SoftSetBoolArg("-staking", staking());
        fIsStakingEnabled = staking();
        break;
    case StakingDonation:
        nStakingDonation = value.toInt();
        if (nStakingDonation < 0) {
            nStakingDonation = 0;
        }
        settings.setValue("nStakingDonation", nStakingDonation);
        break;
    case MinStakeInterval:
        nMinStakeInterval = value.toInt();
        settings.setValue("nMinStakeInterval", nMinStakeInterval);
        break;
    case ThinMode:
        settings.setValue("fThinMode", value.toBool());
        break;
    case ThinFullIndex:
        settings.setValue("fThinFullIndex", value.toBool());
        break;
    case ThinIndexWindow:
        settings.setValue("fThinIndexWindow", value.toInt());
        break;
    default:
        break;
    }
    return successful;
}

qint64 OptionsModel::getTransactionFee()
{
    return nTransactionFee;
}

qint64 OptionsModel::getReserveBalance()
{
    return nReserveBalance;
}

void OptionsModel::emitTransactionFeeChanged(qint64 fee) { emit transactionFeeChanged(fee); }
void OptionsModel::emitReserveBalanceChanged(qint64 bal) { emit reserveBalanceChanged(bal); }
