// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "clientmodel.h"
#include "walletmodel.h"
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

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

static const int64_t nClientStartupTime = GetTime();

ClientModel::ClientModel(OptionsModel *optionsModel, WalletModel *walletModel, QObject *parent) :
    ClientModelRemoteSimpleSource(parent), optionsModel(optionsModel), walletModel(walletModel), pollTimer(0)
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
    return blockChangedEvent.numBlocks;
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
    return QDateTime::fromTime_t(blockChangedEvent.lastBlockTime);
}

void ClientModel::updateNumBlocks(const BlockChangedEvent &blockChangedEvent)
{
    this->blockChangedEvent = blockChangedEvent;
    if (blockInfo().numBlocks() == 0)
        updateTimer();
}

void ClientModel::updateServiceStatus()
{
    const BlockInfo& blockInfo = ClientModelRemoteSimpleSource::blockInfo();

    if (numConnections() == 0)
    {
        QtAndroid::androidService().callMethod<void>("updateNotification", "(Ljava/lang/String;)V", QAndroidJniObject::fromString("Disconnected!").object<jstring>());
        return;
    }
    if (isImporting())
    {
        QtAndroid::androidService().callMethod<void>("updateNotification", "(Ljava/lang/String;)V", QAndroidJniObject::fromString("Importing!").object<jstring>());
        return;
    }

    int nNodeMode = blockInfo.nNodeMode();
    int nNodeState = blockInfo.nNodeState();

    // -- translation (tr()) makes it difficult to neatly pick block/header
    static QString sBlockType = nNodeMode == NT_FULL ? tr("block") : tr("header");
    static QString sBlockTypeMulti = nNodeMode == NT_FULL ? tr("blocks") : tr("headers");

    int count = blockInfo.numBlocks();
    int nTotalBlocks = blockInfo.numBlocksOfPeers();
    QDateTime lastBlockDate = blockInfo.lastBlockTime();

    QString statusMsg;
    if (nNodeMode != NT_FULL
        && nNodeState == NS_GET_FILTERED_BLOCKS)
    {
        float nPercentageDone = blockInfo.nLastFilteredHeight() / (blockInfo.numBlocksOfPeers() * 0.01f);
        statusMsg += tr("(%2% done).").arg(nPercentageDone);
        count = blockInfo.nLastFilteredHeight();
    }
    else
    {
        if (count < nTotalBlocks)
        {
            int nRemainingBlocks = nTotalBlocks - count;
            float nPercentageDone = count / (nTotalBlocks * 0.01f);
            statusMsg += tr("(%1%)").arg(nPercentageDone, 0, 'f', 3);

            if (nNodeMode == NT_FULL)
            {
                statusMsg += tr(" ~%n block(s) remaining.", "", nRemainingBlocks);
            } else
            {
                char temp[128];
                snprintf(temp, sizeof(temp), " ~%%n %s remaining.", nRemainingBlocks == 1 ? qPrintable(sBlockType) : qPrintable(sBlockTypeMulti));
                statusMsg += tr(temp, "", nRemainingBlocks);
            }
        }
        else
        {
            statusMsg = tr("%1 %2 received %3.").arg(sBlockType).arg(count).arg(lastBlockDate.addSecs(-1 * blockInfo.nTimeOffset()).toLocalTime().time().toString(Qt::DefaultLocaleShortDate));
        }
    }

    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime().addSecs(blockInfo.nTimeOffset()));
    if (secs < 90*60 && count >= nTotalBlocks && nNodeState != NS_GET_FILTERED_BLOCKS)
    {
        quint64 nWeight = walletModel->stakingInfo().nWeight(), nNetworkWeight = walletModel->stakingInfo().nNetworkWeight(), nNetworkWeightRecent = walletModel->stakingInfo().nNetworkWeightRecent();
        unsigned nEstimateTime = walletModel->stakingInfo().nEstimateTime();
        if (walletModel->stakingInfo().fIsStaking() && nEstimateTime)
        {
            statusMsg = tr("Staking... / Expected time ");
            statusMsg += (nEstimateTime < 60)           ? tr("%1 second(s)").arg(nEstimateTime) : \
                         (nEstimateTime < 60 * 60)      ? tr("%1 minute(s), %2 second(s)").arg(nEstimateTime / 60).arg(nEstimateTime % 60) : \
                         (nEstimateTime < 24 * 60 * 60) ? tr("%1 hour(s), %2 minute(s)").arg(nEstimateTime / (60 * 60)).arg((nEstimateTime % (60 * 60)) / 60) : \
                                                          tr("%1 day(s), %2 hour(s)").arg(nEstimateTime / (60 * 60 * 24)).arg((nEstimateTime % (60 * 60 * 24)) / (60 * 60));
        }
        else
            statusMsg = tr("Up to date") + " / " + statusMsg;
    }
    else
        statusMsg = tr("Synchronizing...") + " " + statusMsg;

    QtAndroid::androidService().callMethod<void>("updateNotification", "(Ljava/lang/String;)V", QAndroidJniObject::fromString(statusMsg).object<jstring>());
}

void ClientModel::updateTimer() {
    // Some quantities (such as number of blocks) change so fast that we don't want to be notified for each change.
    // Periodically check and update with a timer.

    if (blockChangedEvent.numBlocks != blockInfo().numBlocks()
        || blockChangedEvent.numBlocksOfPeers != blockInfo().numBlocksOfPeers()
        || nNodeState == NS_GET_FILTERED_BLOCKS)
    {
        setBlockInfo(BlockInfo(nNodeMode, nNodeMode,
                                    blockChangedEvent.numBlocks, blockChangedEvent.numBlocksOfPeers,
                                    blockChangedEvent.isInitialBlockDownload, QDateTime::fromTime_t(blockChangedEvent.lastBlockTime),
                                    GetTimeOffset(),
                                    0 /**TODO pwalletMain->nLastFilteredHeight**/));

        updateServiceStatus();
    }

    emit bytesChanged(getTotalBytesRecv(), getTotalBytesSent());
}

void ClientModel::updateNumConnections(int numConnections)
{
    LogPrintf("emit numConnectionsChanged(%i)\n", numConnections);
    setNumConnections(numConnections);
    updateServiceStatus();
    // emit numConnectionsChanged(numConnections); QT remoteobjects impl of setNumConnections() emit signal
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
    setBlockInfo(BlockInfo(nNodeMode, nNodeMode,
                                blockChangedEvent.numBlocks, blockChangedEvent.numBlocksOfPeers,
                                blockChangedEvent.isInitialBlockDownload, QDateTime::fromTime_t(blockChangedEvent.lastBlockTime),
                                GetTimeOffset(),
                                0 /**TODO pwalletMain->nLastFilteredHeight**/));
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
    return blockChangedEvent.isInitialBlockDownload;
}

int ClientModel::getNumBlocksOfPeers() const
{
    return blockChangedEvent.numBlocksOfPeers;
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
   QMetaObject::invokeMethod(clientmodel, "updateNumBlocks", Qt::QueuedConnection, Q_ARG(BlockChangedEvent, blockChangedEvent));
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
