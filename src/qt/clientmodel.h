// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include "interface.h"
#include <QObject>
#include <rep_clientmodelremote_source.h>

Q_DECLARE_METATYPE(BlockChangedEvent);

enum BlockSource {
    BLOCK_SOURCE_NONE,
    BLOCK_SOURCE_REINDEX,
    BLOCK_SOURCE_DISK,
    BLOCK_SOURCE_NETWORK
};

enum NumConnections {
    CONNECTIONS_NONE = 0,
    CONNECTIONS_IN   = (1U << 0),
    CONNECTIONS_OUT  = (1U << 1),
    CONNECTIONS_ALL  = (CONNECTIONS_IN | CONNECTIONS_OUT),
};

class ApplicationModel;
class OptionsModel;
class WalletModel;
class PeerTableModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;

QT_BEGIN_NAMESPACE
class QDateTime;
class QTimer;
QT_END_NAMESPACE

/** Model for Bitcoin network client. */
class ClientModel : public ClientModelRemoteSimpleSource
{
    Q_OBJECT
public:
    explicit ClientModel(ApplicationModel *applicationModel, OptionsModel *optionsModel, WalletModel *walletModel, QObject *parent = 0);
    ~ClientModel();

    OptionsModel *getOptionsModel();
    PeerTableModel *getPeerTableModel();

    //! Return number of connections, default is in- and outbound (total)
    int getNumConnections(unsigned int flags = CONNECTIONS_ALL);
    int getNumBlocks() const;

    quint64 getTotalBytesRecv() const;
    quint64 getTotalBytesSent() const;

    QDateTime getLastBlockDate() const;

    //! Return true if client connected to testnet
    bool isTestNet() const;

    //! mode (thin/full) client is running in
    int getClientMode() const;

    //! Return true if core is doing initial block download
    bool inInitialBlockDownload() const;
    //! Return conservative estimate of total number of blocks, or 0 if unknown
    int getNumBlocksOfPeers() const;
    //! Return true if core is importing blocks
    bool isImporting() const;
    //! Return warnings to be displayed in status bar
    QString getStatusBarWarnings() const;

    QString formatFullVersion() const;
    QString formatBuildDate() const;
    QString clientName() const;
    QString formatClientStartupTime() const;

private:
    ApplicationModel *applicationModel;
    OptionsModel *optionsModel;
    WalletModel *walletModel;
    PeerTableModel *peerTableModel;

    BlockChangedEvent blockChangedEvent;

    QTimer *pollTimer;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

signals:
    void bytesChanged(quint64 totalBytesIn, quint64 totalBytesOut);

    //! Asynchronous error notification
    void error(const QString &title, const QString &message, bool modal);

public slots:
    void updateTimer();
    void updateNumBlocks(const BlockChangedEvent &blockChangedEvent);
    void updateNumConnections(int numConnections);
    void updateAlert(const QString &hash, int status);
    void updateServiceStatus();
};

#endif // CLIENTMODEL_H
