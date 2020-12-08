// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

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
    Q_INVOKABLE void scanQRCode();

    /** Show debug dialog */
    Q_INVOKABLE void userAction(QJsonValue action);

    Q_INVOKABLE void openCoinControl();

    Q_INVOKABLE void addRecipient(QString address, QString label, QString narration, qint64 amount, int txnType);
    Q_INVOKABLE void sendCoins(bool fUseCoinControl, QString sChangeAddr);

    Q_INVOKABLE void updateCoinControlAmount(qint64 amount);
    Q_INVOKABLE void updateCoinControlLabels(unsigned int &quantity, qint64 &amount, qint64 &fee, qint64 &afterfee, unsigned int &bytes, QString &priority, QString low, qint64 &change);

    Q_INVOKABLE QVariantMap signMessage(QString address, QString message);

    /** Create a new address or add an existing address to your Address book */
    Q_INVOKABLE void newAddress(QString addressLabel, int addressType, QString address = "", bool send = false);

signals:
    void emitPaste(QString text);
    void emitCoinControlUpdate(unsigned int quantity, qint64 amount, qint64 fee, qint64 afterfee, unsigned int bytes, QString priority, QString low, qint64 change);
    void emitReceipient(QString address, QString label, QString narration, qint64 amount);

    void triggerElement(QString element, QString trigger);
    void networkAlert(QString alert);

    void addRecipientResult(bool result);
    void sendCoinsResult(bool result);

    void newAddressResult(bool success, QString errorMsg, QString address, bool send);
private:
    SpectreGUI *window;
    QWebChannel *webChannel;
    QList<SendCoinsRecipient> recipients;

    void connectSignals();
    void clearRecipients();
    void abortSendCoins(QString message);
};

#endif // SPECTRECLIENTBRIDGE_H
