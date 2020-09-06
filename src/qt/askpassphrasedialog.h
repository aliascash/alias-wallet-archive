// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef ASKPASSPHRASEDIALOG_H
#define ASKPASSPHRASEDIALOG_H

#include <QDialog>

#ifdef ANDROID
#include <rep_walletmodelremote_replica.h>
#include <rep_applicationmodelremote_replica.h>
#endif

namespace Ui {
    class AskPassphraseDialog;
}

/** Multifunctional dialog to ask for passphrases. Used for encryption, unlocking, and changing the passphrase.
 */
class AskPassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        Encrypt,       /**< Ask passphrase twice and encrypt */
        UnlockStaking, /**< Ask passphrase and unlock */
        Unlock,        /**< Ask passphrase and unlock */
        UnlockRescan,  /**< Ask passphrase and unlock */
        ChangePass,    /**< Ask old passphrase + new passphrase twice */
        Decrypt        /**< Ask passphrase and decrypt wallet */
    };

    explicit AskPassphraseDialog(Mode mode, QWidget *parent = 0);
    ~AskPassphraseDialog();

    void accept();

    void setWalletModel(QSharedPointer<WalletModelRemoteReplica> walletModel);
    void setApplicationModel(QSharedPointer<ApplicationModelRemoteReplica> walletModel);

protected:
    void showEvent(QShowEvent *);

private:
    bool evaluate(QRemoteObjectPendingReply<bool> reply, bool showBusyIndicator = true);

    Ui::AskPassphraseDialog *ui;
    Mode mode;
    QSharedPointer<WalletModelRemoteReplica> walletModel;
    QSharedPointer<ApplicationModelRemoteReplica> applicationModel;
    bool fCapsLock;

private slots:
    void textChanged();
    bool event(QEvent *event);
    bool eventFilter(QObject *, QEvent *event);
    void secureClearPassFields();
};

#endif // ASKPASSPHRASEDIALOG_H
