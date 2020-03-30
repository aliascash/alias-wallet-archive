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
#include "ui_interface.h"

#include <QDateTime>
#include <QTimer>

static const int64_t nClientStartupTime = GetTime();

ClientModel::ClientModel(OptionsModel *optionsModel, QObject *parent) :
    QObject(parent), optionsModel(optionsModel), pollTimer(0)
{
    peerTableModel = new PeerTableModel(this);

    // Fetch data from core in separate thread to not block event loop when waiting for the main lock
    CoreInfoWorker *worker = new CoreInfoWorker();
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &CoreInfoWorker::dataFromCore, this, &ClientModel::updateFromCore, Qt::QueuedConnection);
    workerThread.start();

    // Some quantities (such as number of blocks) change so fast that we don't want to be notified for each change.
    // Periodically fetch data from core with a timer.
    pollTimer = new QTimer(this);
    pollTimer->setInterval(MODEL_UPDATE_DELAY);
    pollTimer->start();
    connect(pollTimer, &QTimer::timeout, worker, &CoreInfoWorker::fetchDataFromCore);

    subscribeToCoreSignals();
}

ClientModel::~ClientModel()
{
    unsubscribeFromCoreSignals();
    workerThread.quit();
    workerThread.wait();
}

void CoreInfoWorker::fetchDataFromCore() {
    if (QDateTime::currentMSecsSinceEpoch() - lastExecutionEpochMS < MODEL_UPDATE_DELAY)
        return; // prevent fetchDataFromCore being executed to often (might happen if thread has to wait long time for lock)

    CoreInfoModel coreInfo;
    {
        LOCK(cs_main);

        coreInfo.numBlocks = nBestHeight;
        coreInfo.numBlocksOfPeers = GetNumBlocksOfPeers();
        coreInfo.isInitialBlockDownload = IsInitialBlockDownload();

        if (nNodeMode == NT_FULL)
        {
            if (pindexBest)
                coreInfo.lastBlockTime = pindexBest->GetBlockTime();
            else
                coreInfo.lastBlockTime = GENESIS_BLOCK_TIME;
        }
        else {
            if (pindexBestHeader)
                coreInfo.lastBlockTime = pindexBestHeader->GetBlockTime();
            else
                coreInfo.lastBlockTime = GENESIS_BLOCK_TIME;
        }
    }
    lastExecutionEpochMS = QDateTime::currentMSecsSinceEpoch();

    emit dataFromCore(coreInfo);
}


int ClientModel::getNumConnections(unsigned int flags) const
{
    LOCK(cs_vNodes);
    if (flags == CONNECTIONS_ALL) // Shortcut if we want total
        return vNodes.size();

    int nNum = 0;
    BOOST_FOREACH(CNode* pnode, vNodes)
    if (flags & (pnode->fInbound ? CONNECTIONS_IN : CONNECTIONS_OUT))
        nNum++;

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
    int lastNumBlocks = this->coreInfo.numBlocks;
    int lastBlocksOfPeers = this->coreInfo.numBlocksOfPeers;

    // Update our coreInfo data
    this->coreInfo = coreInfo;

    if (coreInfo.numBlocks != lastNumBlocks
        || coreInfo.numBlocksOfPeers != lastBlocksOfPeers
        || nNodeState == NS_GET_FILTERED_BLOCKS)
    {
        emit numBlocksChanged(coreInfo.numBlocks, coreInfo.numBlocksOfPeers);
    }

    emit bytesChanged(getTotalBytesRecv(), getTotalBytesSent());
}

void ClientModel::updateNumConnections(int numConnections)
{
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
static void NotifyBlocksChanged(ClientModel *clientmodel)
{
    // This notification is too frequent. Don't trigger a signal.
    // Don't remove it, though, as it might be useful later.
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
    uiInterface.NotifyBlocksChanged.connect(boost::bind(NotifyBlocksChanged, this));
    uiInterface.NotifyNumConnectionsChanged.connect(boost::bind(NotifyNumConnectionsChanged, this, _1));
    uiInterface.NotifyAlertChanged.connect(boost::bind(NotifyAlertChanged, this, _1, _2));
}

void ClientModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.NotifyBlocksChanged.disconnect(boost::bind(NotifyBlocksChanged, this));
    uiInterface.NotifyNumConnectionsChanged.disconnect(boost::bind(NotifyNumConnectionsChanged, this, _1));
    uiInterface.NotifyAlertChanged.disconnect(boost::bind(NotifyAlertChanged, this, _1, _2));
}
