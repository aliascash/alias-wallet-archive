// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "askpassphrasedialog.h"
#include "ui_askpassphrasedialog.h"

#include "guiconstants.h"
#include "allocators.h"

#include <QMessageBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QScreen>

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

AskPassphraseDialog* askPassphraseDialogRef;

AskPassphraseDialog::AskPassphraseDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AskPassphraseDialog),
    mode(mode),
    walletModel(0),
    fCapsLock(false),
    fBiometricUnlock(false)
{
    ui->setupUi(this);
#ifdef ANDROID
    askPassphraseDialogRef = this;
    setFixedSize(QGuiApplication::primaryScreen()->availableSize());
#endif
    ui->progressBar->setVisible(false);
    ui->passEdit1->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit2->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit3->setMaxLength(MAX_PASSPHRASE_SIZE);

    // Setup Caps Lock detection.
    ui->passEdit1->installEventFilter(this);
    ui->passEdit2->installEventFilter(this);
    ui->passEdit3->installEventFilter(this);

    ui->stakingCheckBox->hide();

    switch(mode)
    {
        case Encrypt: // Ask passphrase x2
            ui->passLabel1->hide();
            ui->passEdit1->hide();
            ui->warningLabel->setText(tr("Enter the new passphrase to the wallet.<br/>Please use a passphrase of <b>10 or more random characters</b>, or <b>eight or more words</b>."));
            setWindowTitle(tr("Encrypt wallet"));
            break;
        case UnlockRescan:
            ui->stakingCheckBox->setText(tr("Keep wallet unlocked for staking."));
        case UnlockStaking:
            ui->stakingCheckBox->show();
            // fallthru
        case UnlockLogin:
        case Unlock:
            ui->passLabel2->hide();
            ui->passEdit2->hide();
            ui->passLabel3->hide();
            ui->passEdit3->hide();
            if (mode==UnlockRescan)
                ui->warningLabel->setText(tr("Your wallet contains locked ATXOs for which its spending state can only be determinate with your private key. Your <b>private ALIAS balance might be shown wrong</b>."));
            else if (mode==UnlockLogin)
                ui->warningLabel->setText(tr("<b>Alias Wallet Login</b>"));
            else
                ui->warningLabel->setText(tr("This operation needs your wallet passphrase to unlock the wallet."));
            setWindowTitle(tr("Unlock wallet"));
            break;
        case Decrypt:   // Ask passphrase
            ui->warningLabel->setText(tr("This operation needs your wallet passphrase to decrypt the wallet."));
            ui->passLabel2->hide();
            ui->passEdit2->hide();
            ui->passLabel3->hide();
            ui->passEdit3->hide();
            setWindowTitle(tr("Decrypt wallet"));
            break;
        case ChangePass: // Ask old passphrase + new passphrase x2
            setWindowTitle(tr("Change passphrase"));
            ui->warningLabel->setText(tr("Enter the old and new passphrase to the wallet."));
            break;
    }

    textChanged();
    connect(ui->passEdit1, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit2, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit3, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

#ifdef ANDROID
    if (mode != Encrypt)
        fBiometricUnlock = QtAndroid::androidActivity().callMethod<jboolean>("startBiometricUnlock", "()Z");
#endif
}

AskPassphraseDialog::~AskPassphraseDialog()
{
    secureClearPassFields();
    askPassphraseDialogRef = 0;
    delete ui;
}

void AskPassphraseDialog::setWalletModel(QSharedPointer<WalletModelRemoteReplica> model)
{
    this->walletModel = model;
    ui->stakingCheckBox->setChecked(mode == UnlockStaking || model->encryptionInfo().fWalletUnlockStakingOnly());
}

void AskPassphraseDialog::setApplicationModel(QSharedPointer<ApplicationModelRemoteReplica> model)
{
    this->applicationModel = model;
}

bool AskPassphraseDialog::evaluate(QRemoteObjectPendingReply<bool> reply, bool showBusyIndicator)
{
    setEnabled(false);
    if (showBusyIndicator) ui->progressBar->setVisible(true);
    bool result = reply.waitForFinished(300000) && reply.returnValue();
    if (showBusyIndicator) ui->progressBar->setVisible(false);
    setEnabled(!result);
    return result;
}


void AskPassphraseDialog::accept()
{
    if(!walletModel || !walletModel->isInitialized() || !walletModel->isReplicaValid() ||
       !applicationModel || !applicationModel->isInitialized() || !applicationModel->isReplicaValid())
        return;

    QString oldpass = ui->passEdit1->text();
    QString newpass1 = ui->passEdit2->text();
    QString newpass2 = ui->passEdit3->text();

    secureClearPassFields();

    switch(mode)
    {
    case Encrypt: {
        if(newpass1.isEmpty() || newpass2.isEmpty())
        {
            // Cannot encrypt with empty passphrase
            break;
        }
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm wallet encryption"),
                 tr("Warning: If you encrypt your wallet and lose your passphrase, you will <b>LOSE ALL OF YOUR COINS</b>!") + "<br><br>" + tr("Are you sure you wish to encrypt your wallet?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
        if(retval == QMessageBox::Yes)
        {
            if(newpass1 == newpass2)
            {
                if(evaluate(walletModel->setWalletEncrypted(true, newpass1)))
                {
                    QMessageBox::warning(this, tr("Wallet encrypted"),
                                         "<qt>" +
                                         tr("Alias will close now to finish the encryption process. "
                                         "Remember that encrypting your wallet cannot fully protect "
                                         "your coins from being stolen by malware infecting your computer.") +
                                         "<br><br><b>" +
                                         tr("IMPORTANT: Any previous backups you have made of your wallet file "
                                         "should be replaced with the newly generated, encrypted wallet file.") +
                                         "</b></qt>");
                    this->applicationModel->requestShutdownCore(NORMAL);
                }
                else
                {
                    QMessageBox::critical(this, tr("Wallet encryption failed"),
                                         tr("Wallet encryption failed due to an internal error. Your wallet was not encrypted."));
                }
                QDialog::accept(); // Success
            }
            else
            {
                QMessageBox::critical(this, tr("Wallet encryption failed"),
                                     tr("The supplied passphrases do not match."));
            }
        }
        else
        {
            QDialog::reject(); // Cancelled
        }
        } break;
    case UnlockRescan:
    case UnlockLogin:
    case UnlockStaking:
    case Unlock:
        if(!evaluate(walletModel->unlockWallet(oldpass, ui->stakingCheckBox->isChecked()), mode == UnlockRescan || mode == UnlockLogin))
        {
            QMessageBox::critical(this, tr("Wallet unlock failed"),
                                  tr("The passphrase entered for the wallet decryption was incorrect."));
            handleBiometricUnlockFailed();
        }
        else
        {
#ifdef ANDROID
            if (!fBiometricUnlock)
                QtAndroid::androidActivity().callMethod<jboolean>("setupBiometricUnlock", "(Ljava/lang/String;)Z",
                                                                  QAndroidJniObject::fromString(oldpass).object<jstring>());
#endif
            QDialog::accept(); // Success
        }
        break;
    case Decrypt:
        if(!evaluate(walletModel->setWalletEncrypted(false, oldpass)))
        {
            QMessageBox::critical(this, tr("Wallet decryption failed"),
                                  tr("The passphrase entered for the wallet decryption was incorrect."));
            handleBiometricUnlockFailed();
        }
        else
        {
#ifdef ANDROID
            QtAndroid::androidActivity().callMethod<void>("clearBiometricUnlock", "()V");
#endif
            QDialog::accept(); // Success
        }
        break;
    case ChangePass:
        if(newpass1 == newpass2)
        {
            if(evaluate(walletModel->changePassphrase(oldpass, newpass1)))
            {
                QMessageBox::information(this, tr("Wallet encrypted"),
                                     tr("Wallet passphrase was successfully changed."));
#ifdef ANDROID
                QtAndroid::androidActivity().callMethod<void>("clearBiometricUnlock", "()V");
#endif
                QDialog::accept(); // Success
            }
            else
            {
                QMessageBox::critical(this, tr("Wallet encryption failed"),
                                     tr("The passphrase entered for the wallet decryption was incorrect."));
                handleBiometricUnlockFailed();
            }
        }
        else
        {
            QMessageBox::critical(this, tr("Wallet encryption failed"),
                                 tr("The supplied passphrases do not match."));
        }
        break;
    }
}

void AskPassphraseDialog::textChanged()
{
    // Validate input, set Ok button to enabled when acceptable
    bool acceptable = false;
    switch(mode)
    {
    case Encrypt: // New passphrase x2
        acceptable = !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    case UnlockStaking:
    case UnlockLogin:
    case UnlockRescan:
    case Unlock: // Old passphrase x1
    case Decrypt:
        acceptable = !ui->passEdit1->text().isEmpty();
        break;
    case ChangePass: // Old passphrase x1, new passphrase x2
        acceptable = !ui->passEdit1->text().isEmpty() && !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}

bool AskPassphraseDialog::event(QEvent *event)
{
    // Detect Caps Lock key press.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            fCapsLock = !fCapsLock;
        }
        if (fCapsLock) {
            ui->capsLabel->setText(tr("Warning: The Caps Lock key is on!"));
        } else {
            ui->capsLabel->clear();
        }
    }
    return QWidget::event(event);
}

bool AskPassphraseDialog::eventFilter(QObject *object, QEvent *event)
{
    /* Detect Caps Lock.
     * There is no good OS-independent way to check a key state in Qt, but we
     * can detect Caps Lock by checking for the following condition:
     * Shift key is down and the result is a lower case character, or
     * Shift key is not down and the result is an upper case character.
     */
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QString str = ke->text();
        if (str.length() != 0) {
            const QChar *psz = str.unicode();
            bool fShift = (ke->modifiers() & Qt::ShiftModifier) != 0;
            if ((fShift && psz->isLower()) || (!fShift && psz->isUpper())) {
                fCapsLock = true;
                ui->capsLabel->setText(tr("Warning: The Caps Lock key is on!"));
            } else if (psz->isLetter()) {
                fCapsLock = false;
                ui->capsLabel->clear();
            }
        }
    }
    return QDialog::eventFilter(object, event);
}

void AskPassphraseDialog::secureClearPassFields()
{
    // Attempt to overwrite text so that they do not linger around in memory
    ui->passEdit1->setText(QString(" ").repeated(ui->passEdit1->text().size()));
    ui->passEdit2->setText(QString(" ").repeated(ui->passEdit2->text().size()));
    ui->passEdit3->setText(QString(" ").repeated(ui->passEdit3->text().size()));

    ui->passEdit1->clear();
    ui->passEdit2->clear();
    ui->passEdit3->clear();
}

void AskPassphraseDialog::showEvent(QShowEvent *e)
{
#ifdef ANDROID
    resize(QGuiApplication::primaryScreen()->availableSize());
#endif
    QDialog::showEvent(e);
}

void AskPassphraseDialog::handleBiometricUnlockFailed()
{
#ifdef ANDROID
    if (fBiometricUnlock)
    {
        ui->passEdit1->setReadOnly(false);
        ui->passEdit1->setDisabled(false);
        QtAndroid::androidActivity().callMethod<void>("clearBiometricUnlock", "()V");
        fBiometricUnlock = false;
    }
#endif
}

void AskPassphraseDialog::serveBiometricPassword(QString walletPassword)
{
    ui->passEdit1->setText(walletPassword);
    ui->passEdit1->setReadOnly(true);
    ui->passEdit1->setDisabled(true);
    if (ui->stakingCheckBox->isHidden() && mode != ChangePass)
        accept();
}

#ifdef ANDROID
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_org_alias_wallet_AliasActivity_serveWalletPassword(JNIEnv *env, jobject obj, jstring walletPassword)
{
    const char *walletPasswordStr = env->GetStringUTFChars(walletPassword, NULL);
    Q_UNUSED (obj)
    if (askPassphraseDialogRef)
        QMetaObject::invokeMethod(askPassphraseDialogRef, "serveBiometricPassword", Qt::QueuedConnection,
                                  Q_ARG(QString, walletPasswordStr));
    else {
        qDebug() << "serveWalletPassword could no be proccessed because askPassphraseDialogRef is invalid";
    }
    env->ReleaseStringUTFChars(walletPassword, walletPasswordStr);
    return;
}
#ifdef __cplusplus
}
#endif
#endif
