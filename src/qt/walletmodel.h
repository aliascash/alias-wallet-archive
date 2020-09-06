// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2011 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include <QObject>
#include <vector>
#include <map>

#include "allocators.h" /* for SecureString */
#include "stealth.h"
#include <rep_walletmodelremote_source.h>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;
class CKeyID;
class CPubKey;
class COutput;
class COutPoint;
class uint256;
class CCoinControl;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

using EncryptionStatus = EncryptionStatusEnum::EncryptionStatus;

/** Interface to Bitcoin wallet from Qt view code. */
class WalletModel : public WalletModelRemoteSimpleSource
{
    Q_OBJECT

public:
    explicit WalletModel(CWallet *wallet, OptionsModel *optionsModel, QObject *parent = 0);
    ~WalletModel();

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    qint64 getSpectreBalance() const;
    qint64 getStake() const;
    qint64 getSpectreStake() const;
    qint64 getUnconfirmedBalance() const;
    qint64 getUnconfirmedSpectreBalance() const;
    qint64 getImmatureBalance() const;
    qint64 getImmatureSpectreBalance() const;
    int getNumTransactions() const;
    EncryptionStatus getEncryptionStatus() const;

    // Check address for validity
    bool validateAddress(const QString &address);
    // Sign Message with owned address
    QVariantMap signMessage(const QString &address, const QString &message);

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(const qint64 feeApproval, const QList<SendCoinsRecipient> &recipients, const QList<OutPoint> &coins);
    SendCoinsReturn sendCoinsAnon(const qint64 feeApproval, const QList<SendCoinsRecipient> &recipients, const QList<OutPoint> &coins);

    // Wallet encryption
    bool setWalletEncrypted(bool encrypted, const QString &passphrase);
    bool lockWallet();
    bool unlockWallet(const QString &passPhrase, const bool fStakingOnly);
    bool changePassphrase(const QString &oldPass, const QString &newPass);
    // Wallet backup
    bool backupWallet(const QString &filename);

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy operator and constructor transfer the context
        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        WalletModel *wallet;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying

        void CopyFrom(const UnlockContext& rhs);
    };

    enum UnlockMode { standard, rescan };
    UnlockContext requestUnlock(UnlockMode unlockMode = standard);
    int fUnlockRescanRequested;

    bool getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    void getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs);
    void listCoins(std::map<QString, std::vector<std::pair<COutput,bool>> >& mapCoins) const;
    bool getStealthAddress(const QString &address, CStealthAddress& stealthAddressOut) const;
    bool isLockedCoin(uint256 hash, unsigned int n) const;
    void lockCoin(COutPoint& output);
    void unlockCoin(COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& vOutpts);
    int countLockedAnonOutputs();

    void emitBalanceChanged(qint64 balance, qint64 spectreBal, qint64 stake, qint64 spectreStake, qint64 unconfirmed, qint64 spectreUnconfirmed, qint64 immature, qint64 spectreImmature);
    void emitNumTransactionsChanged(int count);
    void emitRequireUnlock(UnlockMode mode);
    void emitError(const QString &title, const QString &message, bool modal);
    void checkBalanceChanged(bool force = false);

    void updateStakingInfo();

private:
    CWallet *wallet;

    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;

    QTimer *pollTimer;

    // Cache some values to be able to detect changes
    qint64 cachedBalance;
    qint64 cachedSpectreBal;
    qint64 cachedStake;
    qint64 cachedSpectreStake;
    qint64 cachedUnconfirmedBalance;
    qint64 cachedUnconfirmedSpectreBalance;
    qint64 cachedImmatureBalance;
    qint64 cachedImmatureSpectreBalance;
    qint64 cachedNumTransactions;
    int cachedNumBlocks;
    bool fForceCheckBalanceChanged;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

public slots:
    /* Wallet status might have changed */
    void updateStatus(bool force = false);
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString &hash, int status);
    /* New, updated or removed address book entry */
    void updateAddressBook(const QString &address, const QString &label, bool isMine, int status, bool fManual);
    /* Current, immature or unconfirmed balance might have changed - emit 'balanceChanged' if so */
    void pollBalanceChanged();

signals:
    // Signal that balance in wallet changed
    void balanceChanged(qint64 balance, qint64 spectreBal, qint64 stake, qint64 spectreStake, qint64 unconfirmed, qint64 spectreUnconfirmed, qint64 immature, qint64 spectreImmature);

    // Number of transactions in wallet changed
    void numTransactionsChanged(int count);

    // Encryption status of wallet changed
    void encryptionStatusChanged(int status);

    // Signal emitted when wallet needs to be unlocked
    // It is valid behaviour for listeners to keep the wallet locked after this signal;
    // this means that the unlocking failed or was cancelled.
    void requireUnlock(WalletModel::UnlockMode mode);

    // Asynchronous error notification
    void error(const QString &title, const QString &message, bool modal);
};


#endif // WALLETMODEL_H
