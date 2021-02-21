// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2011 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef SPECTREBRIDGE_H
#define SPECTREBRIDGE_H

class ClientModel;
class TransactionTableModel;
class AddressTableModel;
class TransactionModel;
class QSortFilterProxyModel;
class WalletModel;
class BlockExplorerModel;
class AddressModel;
class MessageModel;
class MessageThread;
class ApplicationModelRemoteSource;

#include <stdint.h>
#include <util.h>
#include <QObject>
#include <QModelIndex>
#include <QThread>
#include <QStringList>
#include <QJsonValue>
#include <QJsonArray>

#include <QWebChannel>
#include "websocketclientwrapper.h"
#include "websockettransport.h"
#include <QWebSocketServer>

#include <rep_addressmodelremote_source.h>

class TransactionModel : public QObject
{
    Q_OBJECT

public:
    TransactionModel(QObject *parent = 0);
    ~TransactionModel();
    void init(ClientModel * clientModel, TransactionTableModel * transactionTableModel);
    QVariantMap addTransaction(int row);
    void populateRows(int start, int end, int max = 0);
    void populatePage();
    QSortFilterProxyModel * getModel();
    bool isRunning();

signals:
    void emitTransactions(const QVariantList & transactions, bool reset = false);

private:
    ClientModel *clientModel;
    QSortFilterProxyModel *ttm;
    QStringList visibleTransactions;
    QVariantMap transactionsBuffer;
    int numRows;
    int rowsPerPage;
    bool running;

    bool prepare();
};


class AddressModel : public AddressModelRemoteSimpleSource
{
    Q_OBJECT

public:
    AddressTableModel *atm;

    QVariantMap addAddress(int row);
    void poplateRows(int start, int end);
    void populateAddressTable();
    bool isRunning();

    NewAddressResult newSendAddress(int addressType, const QString & label, const QString & address);
    NewAddressResult newReceiveAddress(int addressType, const QString & label);

signals:
    void emitAddresses(const QVariantList & addresses, bool reset = false);

private:
    NewAddressResult fromAddRow(const QString &addRowReturn);

    bool running;
};


class SpectreBridge : public QObject
{
    Q_OBJECT

    /** Information about the client */
    Q_PROPERTY(QVariantMap info READ getInfo NOTIFY infoChanged)
public:
    explicit SpectreBridge(QWebChannel *webChannel, QObject *parent = 0);
    ~SpectreBridge();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void setApplicationModel(ApplicationModelRemoteSource *applicationModel);
    void setTransactionModel();
    AddressModel* getAddressModel() const { return addressModel; }

public slots:
    Q_INVOKABLE void jsReady();

    /** Get the label belonging to an address */
    Q_INVOKABLE QString getAddressLabel(QString address);
    Q_INVOKABLE void getAddressLabelAsync(QString address);
    Q_INVOKABLE void getAddressLabelForSelectorAsync(QString address, QString selector, QString fallback = "");

    /** Get the full transaction details */
    Q_INVOKABLE void transactionDetails(QString txid);
    /** Get the pubkey for an address */
    Q_INVOKABLE QString getPubKey(QString address);

    /** Show debug dialog */
    Q_INVOKABLE QJsonValue userAction(QJsonValue action);

    Q_INVOKABLE void populateTransactionTable();

    Q_INVOKABLE void updateAddressLabel(QString address, QString label);
    Q_INVOKABLE void validateAddress(QString own);
    Q_INVOKABLE bool deleteAddress(QString address);

    Q_INVOKABLE QVariantMap listAnonOutputs();

    Q_INVOKABLE void findBlock(QString searchID);
    Q_INVOKABLE void listLatestBlocks();
    Q_INVOKABLE void blockDetails(QString blkid);
    Q_INVOKABLE void listTransactionsForBlock(QString blkid);
    Q_INVOKABLE void txnDetails(QString blkHash, QString txnHash);

    Q_INVOKABLE QVariantMap verifyMessage(QString address, QString message, QString signature);

    Q_INVOKABLE void importFromMnemonic(QString inMnemonic, QString inPassword, QString inLabel, bool fBip44 = false, quint64 nCreateTime = 0);
    Q_INVOKABLE void getNewMnemonic(QString password, QString language);
    Q_INVOKABLE void extKeyAccList();
    Q_INVOKABLE void extKeyList();
    Q_INVOKABLE void extKeyImport(QString inKey, QString inLabel, bool fBip44 = false, quint64 nCreateTime = 0);
    Q_INVOKABLE void extKeySetDefault(QString extKeyID);
    Q_INVOKABLE void extKeySetMaster(QString extKeyID);
    Q_INVOKABLE void extKeySetActive(QString extKeySetActive, QString isActive);

    Q_INVOKABLE void getOptions();

signals:
    void emitTransactions(QVariantList transactions, bool reset);
    void emitAddresses(QVariantList addresses);
    void emitAddressBookReturn(QString address, QString label);
    void emitReceipient(QString address, QString label, QString narration, qint64 amount);
    void triggerElement(QString element, QString trigger);
    void networkAlert(QString alert);
    void infoChanged();

    void validateAddressResult(bool result);
    void transactionDetailsResult(QString result);

    void findBlockResult(QVariantMap result);
    void listLatestBlocksResult(QVariantMap result);
    void blockDetailsResult(QVariantMap result);
    void listTransactionsForBlockResult(QString blkHash, QVariantMap result);
    void txnDetailsResult(QVariantMap result);

    void importFromMnemonicResult(QVariantMap result);
    void getNewMnemonicResult(QVariantMap result);
    void extKeyAccListResult(QVariantMap result);
    void extKeyListResult(QVariantMap result);
    void extKeyImportResult(QVariantMap result);
    void extKeySetDefaultResult(QVariantMap result);
    void extKeySetMasterResult(QVariantMap result);
    void extKeySetActiveResult(QVariantMap result);

    void getAddressLabelResult(QString result);
    void getAddressLabelToSendBalanceResult(QString result);
    void getAddressLabelForSelectorResult(QString result, QString selector, QString fallback);

    void getOptionResult(QVariant result);

    void listAnonOutputsResult(QVariantMap result);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    TransactionModel *transactionModel;
    AddressModel *addressModel;
    ApplicationModelRemoteSource * applicationModel;
    QVariantMap *info;
    QThread *async;
    QWebChannel *webChannel;

    friend class SpectreGUI;

    inline QVariantMap getInfo() const { return *info; }

    void populateOptions();
    void populateAddressTable();
    void connectSignals();

    void appendMessage(int row);

private slots:
    void updateTransactions(QModelIndex topLeft, QModelIndex bottomRight);
    void updateAddresses(QModelIndex topLeft, QModelIndex bottomRight);
    void insertTransactions(const QModelIndex &parent, int start, int end);
    void insertAddresses(const QModelIndex &parent, int start, int end);

    // TODO Android move to separate class
    void incomingTransaction(const QModelIndex & parent, int start, int end);
};

#endif // SPECTREBRIDGE_H
