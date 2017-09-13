#ifndef SPECTREBRIDGE_H
#define SPECTREBRIDGE_H

class SpectreGUI;
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
class SendCoinsRecipient;

#include <stdint.h>
#include <QObject>
#include <QModelIndex>
#include <QThread>
#include <QStringList>

class TransactionModel : public QObject
{
    Q_OBJECT

public:
    TransactionModel(QObject *parent = 0);
    ~TransactionModel();
    void init(ClientModel * clientModel, TransactionTableModel * transactionTableModel);
    QVariantMap addTransaction(int row);
    void populateRows(int start, int end);
    void populatePage();
    QSortFilterProxyModel * getModel();
    bool isRunning();

signals:
    void emitTransactions(const QVariantList & transactions, bool reset = false);

private:
    ClientModel *clientModel;
    QSortFilterProxyModel *ttm;
    QStringList visibleTransactions;
    int numRows;
    int rowsPerPage;
    bool running;

    bool prepare();
};


class AddressModel : public QObject
{
    Q_OBJECT

public:
    AddressTableModel *atm;

    QVariantMap addAddress(int row);
    void poplateRows(int start, int end);
    void populateAddressTable();
    bool isRunning();

signals:
    void emitAddresses(const QVariantList & addresses, bool reset = false);

private:
    bool running;
};


class MessageThread : public QThread
{
    Q_OBJECT

signals:
    void emitMessages(const QString & messages, bool reset);

public:
    MessageModel *mtm;

    QString addMessage(int row);

protected:
    void run();
};


class SpectreBridge : public QObject
{
    Q_OBJECT

    /** Information about the client */
    Q_PROPERTY(QVariantMap info READ getInfo);
public:
    explicit SpectreBridge(SpectreGUI *window, QObject *parent = 0);
    ~SpectreBridge();

    void setClientModel();
    void setWalletModel();
    void setTransactionModel();
    void setAddressModel();
    void setMessageModel();

    Q_INVOKABLE void jsReady();

    Q_INVOKABLE void copy(QString text);
    Q_INVOKABLE void paste();

    /** Get the label belonging to an address */
    Q_INVOKABLE QString getAddressLabel(QString address);
    /** Create a new address or add an existing address to your Address book */
    Q_INVOKABLE QString newAddress(QString addressLabel, int addressType, QString address = "", bool send = false);
    Q_INVOKABLE QString lastAddressError();
    /** Get the full transaction details */
    Q_INVOKABLE QString transactionDetails(QString txid);
    /** Get the pubkey for an address */
    Q_INVOKABLE QString getPubKey(QString address);

    /** Show debug dialog */
    Q_INVOKABLE QVariantMap userAction(QVariantMap action);

    Q_INVOKABLE void populateTransactionTable();

    Q_INVOKABLE void updateAddressLabel(QString address, QString label);
    Q_INVOKABLE bool validateAddress(QString own);
    Q_INVOKABLE bool deleteAddress(QString address);

    Q_INVOKABLE bool deleteMessage(QString key);
    Q_INVOKABLE bool markMessageAsRead(QString key);

    Q_INVOKABLE void openCoinControl();

    Q_INVOKABLE bool addRecipient(QString address, QString label, QString narration, qint64 amount, int txnType, int nRingSize);
    Q_INVOKABLE bool sendCoins(bool fUseCoinControl, QString sChangeAddr);
    Q_INVOKABLE bool setPubKey(QString address, QString pubkey);
    Q_INVOKABLE bool sendMessage(const QString &address, const QString &message, const QString &from);
    Q_INVOKABLE QString joinGroupChat(QString privkey, QString label);
    Q_INVOKABLE QString createGroupChat(QString label);
    Q_INVOKABLE QVariantList inviteGroupChat(QString address, QVariantList invites, QString from);

    Q_INVOKABLE void updateCoinControlAmount(qint64 amount);
    Q_INVOKABLE void updateCoinControlLabels(unsigned int &quantity, int64_t &amount, int64_t &fee, int64_t &afterfee, unsigned int &bytes, QString &priority, QString low, int64_t &change);

    Q_INVOKABLE QVariantMap listAnonOutputs();

    Q_INVOKABLE QVariantMap findBlock(QString searchID);
    Q_INVOKABLE QVariantMap listLatestBlocks();
    Q_INVOKABLE QVariantMap blockDetails(QString blkid);
    Q_INVOKABLE QVariantMap listTransactionsForBlock(QString blkid);
    Q_INVOKABLE QVariantMap txnDetails(QString blkHash, QString txnHash);

    Q_INVOKABLE QVariantMap signMessage(QString address, QString message);
    Q_INVOKABLE QVariantMap verifyMessage(QString address, QString message, QString signature);

    Q_INVOKABLE QVariantMap importFromMnemonic(QString inMnemonic, QString inPassword, QString inLabel, bool fBip44 = false, int64_t nCreateTime = 0);
    Q_INVOKABLE QVariantMap getNewMnemonic(QString password, QString language);
    Q_INVOKABLE QVariantMap extKeyAccList();
    Q_INVOKABLE QVariantMap extKeyList();
    Q_INVOKABLE QVariantMap extKeyImport(QString inKey, QString inLabel, bool fBip44 = false, int64_t nCreateTime = 0);
    Q_INVOKABLE QVariantMap extKeySetDefault(QString extKeyID);
    Q_INVOKABLE QVariantMap extKeySetMaster(QString extKeyID);
    Q_INVOKABLE QVariantMap extKeySetActive(QString extKeySetActive, QString isActive);

    Q_INVOKABLE QString translateHtmlString(QString string);

signals:
    void emitPaste(QString text);
    void emitTransactions(QVariantList transactions);
    void emitAddresses(QVariantList addresses);
    void emitMessages(QString messages, bool reset);
    void emitMessage(QString id, QString type, qint64 sent, qint64 received, QString label_v, QString label, QString labelTo, QString to, QString from, bool read, QString message);
    void emitCoinControlUpdate(unsigned int quantity, qint64 amount, qint64 fee, qint64 afterfee, unsigned int bytes, QString priority, QString low, qint64 change);
    void emitAddressBookReturn(QString address, QString label);
    void emitReceipient(QString address, QString label, QString narration, qint64 amount);
    void triggerElement(QString element, QString trigger);
    void networkAlert(QString alert);

private:
    SpectreGUI *window;
    TransactionModel *transactionModel;
    AddressModel *addressModel;
    MessageThread *thMessage;
    QList<SendCoinsRecipient> recipients;
    QVariantMap *info;
    QThread *async;

    friend class SpectreGUI;

    inline QVariantMap getInfo() const { return *info; };

    void populateOptions();
    void populateAddressTable();
    void connectSignals();
    void clearRecipients();

    void appendMessage(int row);

private slots:
    void updateTransactions(QModelIndex topLeft, QModelIndex bottomRight);
    void updateAddresses(QModelIndex topLeft, QModelIndex bottomRight);
    void insertTransactions(const QModelIndex &parent, int start, int end);
    void insertAddresses(const QModelIndex &parent, int start, int end);
    void insertMessages(const QModelIndex &parent, int start, int end);

    void appendMessages(QString messages, bool reset);

    void populateMessageTable();

};

#endif // SPECTREBRIDGE_H
