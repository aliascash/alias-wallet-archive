// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2011 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "optionsmodel.h"
#include "bitcoinunits.h"
#include <QSettings>

#include "init.h"
#include "walletdb.h"
#include "guiutil.h"
#include "ringsig.h"

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    Init();
}

void OptionsModel::Init()
{
    QSettings settings;

    int nSettingsVersion = settings.value("nSettingsVersion", 0).toInt();
    if (nSettingsVersion < 1) {
        settings.clear();
        settings.setValue("nSettingsVersion", 1);
    }

    // These are Qt-only settings:
    //nDisplayUnit = settings.value("nDisplayUnit", BitcoinUnits::ALIAS).toInt();
    nDisplayUnit = BitcoinUnits::ALIAS;
    bDisplayAddresses = settings.value("bDisplayAddresses", false).toBool();
    fMinimizeToTray = settings.value("fMinimizeToTray", false).toBool();
    fMinimizeOnClose = settings.value("fMinimizeOnClose", false).toBool();
    nTransactionFee = settings.value("nTransactionFee").toLongLong() >= nMinTxFee ? settings.value("nTransactionFee").toLongLong() : nMinTxFee;
    nReserveBalance = settings.value("nReserveBalance").toLongLong();
    language = settings.value("language", "").toString();
    nRowsPerPage = settings.value("nRowsPerPage", 20).toInt();
    notifications = settings.value("notifications", "*").toStringList();
    visibleTransactions = settings.value("visibleTransactions", "*").toStringList();
    fAutoRingSize = settings.value("fAutoRingSize", false).toBool();
    fAutoRedeemSpectre = settings.value("fAutoRedeemSpectre", false).toBool();
    nMinRingSize = settings.value("nMinRingSize", MIN_RING_SIZE).toInt();
    nMaxRingSize = settings.value("nMaxRingSize", MIN_RING_SIZE).toInt();

    // These are shared with core Bitcoin; we want
    // command-line options to override the GUI settings:
    if (settings.contains("detachDB"))
        SoftSetBoolArg("-detachdb", settings.value("detachDB").toBool());
    if (!language.isEmpty())
        SoftSetArg("-lang", language.toStdString());
    if (settings.contains("fStaking"))
    {
        SoftSetBoolArg("-staking", settings.value("fStaking").toBool());
        fIsStakingEnabled = settings.value("fStaking").toBool();
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

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return GUIUtil::GetStartOnSystemStartup();
        case MinimizeToTray:
            return fMinimizeToTray;
        case MinimizeOnClose:
            return fMinimizeOnClose;
        case Fee:
            return (qint64) nTransactionFee;
        case ReserveBalance:
            return (qint64) nReserveBalance;
        case DisplayUnit:
            return nDisplayUnit;
        case DisplayAddresses:
            return bDisplayAddresses;
        case DetachDatabases:
            return bitdb.GetDetach();
        case Language:
            return settings.value("language", "");
        case RowsPerPage:
            return nRowsPerPage;
        case AutoRingSize:
            return fAutoRingSize;
        case AutoRedeemSpectre:
            return fAutoRedeemSpectre;
        case MinRingSize:
            return nMinRingSize;
        case MaxRingSize:
            return nMaxRingSize;
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
            return notifications;
        case VisibleTransactions:
            return visibleTransactions;
        }
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

bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
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
            //nDisplayUnit = value.toInt();
            //settings.setValue("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(nDisplayUnit);
            break;
        case DisplayAddresses:
            bDisplayAddresses = value.toBool();
            settings.setValue("bDisplayAddresses", bDisplayAddresses);
            emit displayUnitChanged(settings.value("nDisplayUnit", BitcoinUnits::ALIAS).toInt());
            break;
        case DetachDatabases: {
            bool fDetachDB = value.toBool();
            bitdb.SetDetach(fDetachDB);
            settings.setValue("detachDB", fDetachDB);
            }
            break;
        case Language:
            settings.setValue("language", value);

            // As long as the real strings of the choosen language are used to filter transactions and notifications,
            // this workaround is neccessary. Otherwise the transaction list will be empty after a language change and
            // wallet restart.
            visibleTransactions.clear();
            visibleTransactions.append("*");
            settings.setValue("visibleTransactions", visibleTransactions);
            bActivateAllTransactiontypesAfterLanguageSwitch = true;

            notifications.clear();
            notifications.append("*");
            settings.setValue("notifications", notifications);
            bActivateAllNotificationsAfterLanguageSwitch = true;

            break;
        case RowsPerPage: {
            nRowsPerPage = value.toInt();
            settings.setValue("nRowsPerPage", nRowsPerPage);
            emit rowsPerPageChanged(nRowsPerPage);
            }
            break;
        case Notifications: {
            if (bActivateAllNotificationsAfterLanguageSwitch) {
                notifications.clear();
                notifications.append("*");
                bActivateAllNotificationsAfterLanguageSwitch = false;
            } else {
                notifications = value.toStringList();
            }
            settings.setValue("notifications", notifications);
            }
            break;
        case VisibleTransactions: {
            if (bActivateAllTransactiontypesAfterLanguageSwitch) {
                visibleTransactions.clear();
                visibleTransactions.append("*");
                bActivateAllTransactiontypesAfterLanguageSwitch = false;
            } else {
                visibleTransactions = value.toStringList();
            }
            settings.setValue("visibleTransactions", visibleTransactions);
            emit visibleTransactionsChanged(visibleTransactions);
            }
            break;
        case AutoRingSize: {
            fAutoRingSize = value.toBool();
            settings.setValue("fAutoRingSize", fAutoRingSize);
            }
            break;
        case AutoRedeemSpectre: {
            fAutoRedeemSpectre = value.toBool();
            settings.setValue("fAutoRedeemSpectre", fAutoRedeemSpectre);
            }
            break;
        case MinRingSize: {
            nMinRingSize = value.toInt();
            settings.setValue("nMinRingSize", nMinRingSize);
            }
            break;
        case MaxRingSize: {
            nMaxRingSize = value.toInt();
            settings.setValue("nMaxRingSize", nMaxRingSize);
            }
            break;
        case Staking:
            settings.setValue("fStaking", value.toBool());
            SoftSetBoolArg("-staking", value.toBool());
            fIsStakingEnabled = value.toBool();
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
    }
    emit dataChanged(index, index);

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

bool OptionsModel::getMinimizeToTray()
{
    return fMinimizeToTray;
}

bool OptionsModel::getMinimizeOnClose()
{
    return fMinimizeOnClose;
}

int OptionsModel::getDisplayUnit()
{
    return nDisplayUnit;
}

bool OptionsModel::getDisplayAddresses()
{
    return bDisplayAddresses;
}

int OptionsModel::getRowsPerPage() { return nRowsPerPage; }
QStringList OptionsModel::getNotifications() { return notifications; }
QStringList OptionsModel::getVisibleTransactions() { return visibleTransactions; }
bool OptionsModel::getAutoRingSize() { return fAutoRingSize; }
bool OptionsModel::getAutoRedeemSpectre() { return fAutoRedeemSpectre; }
int OptionsModel::getMinRingSize() { return nMinRingSize; }
int OptionsModel::getMaxRingSize() { return nMaxRingSize; }

void OptionsModel::emitDisplayUnitChanged(int unit) { emit displayUnitChanged(unit); }
void OptionsModel::emitTransactionFeeChanged(qint64 fee) { emit transactionFeeChanged(fee); }
void OptionsModel::emitReserveBalanceChanged(qint64 bal) { emit reserveBalanceChanged(bal); }
void OptionsModel::emitRowsPerPageChanged(int rows) { emit rowsPerPageChanged(rows); }
void OptionsModel::emitVisibleTransactionsChanged(QStringList txns) { emit visibleTransactionsChanged(txns); }
