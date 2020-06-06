// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPECTRECLIENTBRIDGE_H
#define SPECTRECLIENTBRIDGE_H

class SpectreGUI;
class SendCoinsRecipient;

#include <stdint.h>
#include <util.h>
#include <QObject>
#include <QStringList>
#include <QJsonValue>
#include <QJsonArray>

#include <QWebChannel>
#include "websocketclientwrapper.h"
#include "websockettransport.h"
#include <QWebSocketServer>

#include <rep_clientmodelremote_replica.h>
#include <rep_walletmodelremote_replica.h>

class SpectreClientBridge : public QObject
{
    Q_OBJECT

public:
    explicit SpectreClientBridge(SpectreGUI *window, QWebChannel *webChannel, QObject *parent = 0);
    ~SpectreClientBridge();

    void setWalletModel(QSharedPointer<WalletModelRemoteReplica> *walletModel);
    void setClientModel(QSharedPointer<ClientModelRemoteReplica> *clientModel);

public slots:
    Q_INVOKABLE void copy(QString text);
    Q_INVOKABLE void paste();
    Q_INVOKABLE void urlClicked(const QString link);

    /** Show debug dialog */
    Q_INVOKABLE void userAction(QJsonValue action);

    Q_INVOKABLE void openCoinControl();

    Q_INVOKABLE void addRecipient(QString address, QString label, QString narration, qint64 amount, int txnType, int nRingSize);
    Q_INVOKABLE void sendCoins(bool fUseCoinControl, QString sChangeAddr);

    Q_INVOKABLE void updateCoinControlAmount(qint64 amount);
    Q_INVOKABLE void updateCoinControlLabels(unsigned int &quantity, qint64 &amount, qint64 &fee, qint64 &afterfee, unsigned int &bytes, QString &priority, QString low, qint64 &change);

    Q_INVOKABLE QVariantMap signMessage(QString address, QString message);

signals:
    void emitPaste(QString text);
    void emitCoinControlUpdate(unsigned int quantity, qint64 amount, qint64 fee, qint64 afterfee, unsigned int bytes, QString priority, QString low, qint64 change);
    void emitReceipient(QString address, QString label, QString narration, qint64 amount);

    void triggerElement(QString element, QString trigger);
    void networkAlert(QString alert);

    void addRecipientResult(bool result);
    void sendCoinsResult(bool result);
private:
    SpectreGUI *window;
    QWebChannel *webChannel;
    QList<SendCoinsRecipient> recipients;

    void connectSignals();
    void clearRecipients();
    void abortSendCoins(QString message);
};

#endif // SPECTRECLIENTBRIDGE_H
