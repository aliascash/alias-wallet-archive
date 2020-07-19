// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>
#include <QStringList>

#include <rep_optionsmodelremote_source.h>

/** Interface from Qt to configuration data structure for Bitcoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public OptionsModelRemoteSimpleSource
{
    Q_OBJECT

public:
    explicit OptionsModel(QObject *parent = 0);

    enum OptionID {
        /// Main Options
        Fee,                 /**< Transaction Fee. qint64 - Optional transaction fee per kB that helps make sure your transactions are processed quickly.
                               *< Most transactions are 1 kB. Fee 0.01 recommended. 0.0001 XSPEC */
        ReserveBalance,      /**< Reserve Balance. qint64 - Reserved amount does not participate in staking and is therefore spendable at any time. */
        StartAtStartup,      /**< Default Transaction Fee. bool */
        DetachDatabases,     /**< Default Transaction Fee. bool */
        Staking,             /**< Default Transaction Fee. bool */
        StakingDonation,
        MinStakeInterval,
        ThinMode,            /**< Default Transaction Fee. bool */
        ThinFullIndex,
        ThinIndexWindow,
        AutoRingSize,        /**< Default Transaction Fee. bool */
        AutoRedeemSpectre,    /**< Default Transaction Fee. bool */
        MinRingSize,         /**< Default Transaction Fee. int */
        MaxRingSize,         /**< Default Transaction Fee. int */
        /// Network Related Options
        MapPortUPnP,         /**< Default Transaction Fee. bool */
        ProxyUse,            /**< Default Transaction Fee. bool */
        ProxyIP,             // QString
        ProxyPort,           // int
        ProxySocksVersion,   // int
        /// Window Options
        MinimizeToTray,      /**< Default Transaction Fee. bool */
        MinimizeOnClose,     /**< Default Transaction Fee. bool */
        /// Display Options
        Language,            // QString
        DisplayUnit,         // Bitcoinnits::Unit
        DisplayAddresses,    /**< Default Transaction Fee. bool */
        RowsPerPage,         // int
        Notifications,       // QStringList
        VisibleTransactions, // QStringList
        OptionIDRowCount,
    };

    QString optionIDName(int row);
    int optionNameID(QString name);

    void Init();

    int rowCount() const;
    QVariant data(const int row) const;
    bool setData(const int row, const QVariant & value);

    /* Explicit getters */
    qint64 getTransactionFee();
    qint64 getReserveBalance();

    void emitTransactionFeeChanged(qint64);
    void emitReserveBalanceChanged(qint64);

signals:
    void transactionFeeChanged(qint64);
    void reserveBalanceChanged(qint64);
};

#endif // OPTIONSMODEL_H
