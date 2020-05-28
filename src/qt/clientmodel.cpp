// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "clientmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "peertablemodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "alert.h"
#include "main.h"
#include "interface.h"

#include <QDateTime>
#include <QTimer>

static const int64_t nClientStartupTime = GetTime();

ClientModel::ClientModel(OptionsModel *optionsModel, QObject *parent) :
    ClientModelRemoteSource(parent), optionsModel(optionsModel), pollTimer(0)
{
    peerTableModel = new PeerTableModel(this);

    // Some quantities (such as number of blocks) change so fast that we don't want to be notified for each change.
    // Periodically fetch data from core with a timer.
    pollTimer = new QTimer(this);
    pollTimer->setInterval(MODEL_UPDATE_DELAY);
    pollTimer->start();
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(updateTimer()));

    subscribeToCoreSignals();
}

ClientModel::~ClientModel()
{
    unsubscribeFromCoreSignals();
}

int ClientModel::getNumConnections(unsigned int flags)
{
    LOCK(cs_vNodes);
    if (flags == CONNECTIONS_ALL) {// Shortcut if we want total
        LogPrintf("getNumConnections(CONNECTIONS_ALL): %i\n", vNodes.size());
        return vNodes.size();
    }

    int nNum = 0;
    BOOST_FOREACH(CNode* pnode, vNodes)
    if (flags & (pnode->fInbound ? CONNECTIONS_IN : CONNECTIONS_OUT))
        nNum++;

    LogPrintf("getNumConnections(%s): %i\n", flags & CONNECTIONS_IN ? "CONNECTIONS_IN ": "CONNECTIONS_OUT", nNum);
    return nNum;
}

int ClientModel::getNumBlocks() const
{
    return coreInfo.numBlocks;
}

quint64 ClientModel::getTotalBytesRecv() const
{
    return CNode::GetTotalBytesRecv();
}

quint64 ClientModel::getTotalBytesSent() const
{
    return CNode::GetTotalBytesSent();
}

QDateTime ClientModel::getLastBlockDate() const
{
    return QDateTime::fromTime_t(coreInfo.lastBlockTime);
}

void ClientModel::updateFromCore(const CoreInfoModel &coreInfo) {
    this->coreInfo = coreInfo;
}

void ClientModel::updateTimer() {
    if (coreInfo.numBlocks != lastPublishedCoreInfo.numBlocks
        || coreInfo.numBlocksOfPeers != lastPublishedCoreInfo.numBlocksOfPeers
        || nNodeState == NS_GET_FILTERED_BLOCKS)
    {
        lastPublishedCoreInfo = coreInfo;
        emit numBlocksChanged(coreInfo.numBlocks, coreInfo.numBlocksOfPeers);
    }

    emit bytesChanged(getTotalBytesRecv(), getTotalBytesSent());
}

void ClientModel::updateNumConnections(int numConnections)
{
    LogPrintf("emit numConnectionsChanged(%i)\n", numConnections);
    emit numConnectionsChanged(numConnections);
}

void ClientModel::updateAlert(const QString &hash, int status)
{
    // Show error message notification for new alert
    if (status == CT_NEW)
    {
        uint256 hash_256;
        hash_256.SetHex(hash.toStdString());
        CAlert alert = CAlert::getAlertByHash(hash_256);
        if (!alert.IsNull())
        {
            emit error(tr("Network Alert"), QString::fromStdString(alert.strStatusBar), false);
        };
    };

    // Emit a numBlocksChanged when the status message changes,
    // so that the view recomputes and updates the status bar.
    emit numBlocksChanged(getNumBlocks(), getNumBlocksOfPeers());
}

bool ClientModel::isTestNet() const
{
    return fTestNet;
}

int ClientModel::getClientMode() const
{
    return nNodeMode;
}

bool ClientModel::inInitialBlockDownload() const
{
    return coreInfo.isInitialBlockDownload;
}

int ClientModel::getNumBlocksOfPeers() const
{
    return coreInfo.numBlocksOfPeers;
}
bool ClientModel::isImporting() const
{
    return fImporting;
}

QString ClientModel::getStatusBarWarnings() const
{
    return QString::fromStdString(GetWarnings("statusbar"));
}

OptionsModel *ClientModel::getOptionsModel()
{
    return optionsModel;
}

PeerTableModel *ClientModel::getPeerTableModel()
{
    return peerTableModel;
}

QString ClientModel::formatFullVersion() const
{
    return QString::fromStdString(FormatFullVersion());
}

QString ClientModel::formatBuildDate() const
{
    return QString::fromStdString(CLIENT_DATE);
}

QString ClientModel::clientName() const
{
    return QString::fromStdString(CLIENT_NAME);
}

QString ClientModel::formatClientStartupTime() const
{
    return QDateTime::fromTime_t(nClientStartupTime).toString();
}

// Handlers for core signals
static void NotifyBlocksChanged(ClientModel *clientmodel, const BlockChangedEvent &blockChangedEvent)
{
   CoreInfoModel coreInfoModel = {blockChangedEvent.numBlocks, blockChangedEvent.numBlocksOfPeers, blockChangedEvent.isInitialBlockDownload, blockChangedEvent.lastBlockTime};
   QMetaObject::invokeMethod(clientmodel, "updateFromCore", Qt::QueuedConnection, Q_ARG(CoreInfoModel, coreInfoModel));
}

static void NotifyNumConnectionsChanged(ClientModel *clientmodel, int newNumConnections)
{
    // Too noisy: LogPrintf("NotifyNumConnectionsChanged %i\n", newNumConnections);
    QMetaObject::invokeMethod(clientmodel, "updateNumConnections", Qt::QueuedConnection,
                              Q_ARG(int, newNumConnections));
}

static void NotifyAlertChanged(ClientModel *clientmodel, const uint256 &hash, ChangeType status)
{
    LogPrintf("NotifyAlertChanged %s status=%i\n", hash.GetHex().c_str(), status);
    QMetaObject::invokeMethod(clientmodel, "updateAlert", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(hash.GetHex())),
                              Q_ARG(int, status));
}

void ClientModel::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.NotifyBlocksChanged.connect(boost::bind(NotifyBlocksChanged, this, _1));
    uiInterface.NotifyNumConnectionsChanged.connect(boost::bind(NotifyNumConnectionsChanged, this, _1));
    uiInterface.NotifyAlertChanged.connect(boost::bind(NotifyAlertChanged, this, _1, _2));
}

void ClientModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.NotifyBlocksChanged.disconnect(boost::bind(NotifyBlocksChanged, this, _1));
    uiInterface.NotifyNumConnectionsChanged.disconnect(boost::bind(NotifyNumConnectionsChanged, this, _1));
    uiInterface.NotifyAlertChanged.disconnect(boost::bind(NotifyAlertChanged, this, _1, _2));
}
