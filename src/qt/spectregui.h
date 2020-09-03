// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef SPECTREGUI_H
#define SPECTREGUI_H

#include <QMainWindow>
#include <QtWebView>
#include <QWebChannel>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QModelIndex>
#include <QSplashScreen>

#include "spectreclientbridge.h"
#include "walletmodel.h"
#include "askpassphrasedialog.h"
#include "rpcconsole.h"

#include <stdint.h>

#include <rep_clientmodelremote_replica.h>
#include <rep_applicationmodelremote_replica.h>
#include <rep_walletmodelremote_replica.h>
#include <rep_addressmodelremote_replica.h>
#include <rep_optionsmodelremote_replica.h>

class TransactionTableModel;
class ClientModel;
class WalletModel;
class MessageModel;
class Notificator;

QT_BEGIN_NAMESPACE
class QLabel;
class QMenuBar;
class QToolBar;
class QUrl;
class QProgressDialog;
QT_END_NAMESPACE

static const int WEBSOCKETPORT = 52471;
static const int WEBSOCKETPORT_TESTNET = 52571;

/**
  Spectre GUI main class. This class represents the main window of the Spectre UI. It communicates with both the client and
  wallet models to give the user an up-to-date view of the current core state.
*/
class SpectreGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit SpectreGUI(QSharedPointer<ApplicationModelRemoteReplica> applicationModel,
                        QSharedPointer<ClientModelRemoteReplica> clientModel,
                        QSharedPointer<WalletModelRemoteReplica> walletModel,
                        QSharedPointer<AddressModelRemoteReplica> addressModel,
                        QSharedPointer<OptionsModelRemoteReplica> optionsModelPtr,
                        QWidget *parent = 0);
    ~SpectreGUI();

//    /** Set the client model.
//        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
//    */
//    void setClientModel(ClientModel *clientModel);
//    /** Set the wallet model.
//        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
//        functionality.
//    */
//    void setWalletModel(WalletModel *walletModel);
//    /** Set the message model.
//        The message model represents encryption message database, and offers access to the list of messages, address book and sending
//        functionality.
//    */
//    void setMessageModel(MessageModel *messageModel);

    void setSplashScreen(QSplashScreen* splash);

    void loadIndex(QString webSocketToken);

    void runJavaScript(QString javascriptCode);

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(SpectreGUI *window, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy operator and constructor transfer the context
        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        SpectreGUI *window;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying

        void CopyFrom(const UnlockContext& rhs);
    };

    enum UnlockMode { standard, rescan };
    UnlockContext requestUnlock(UnlockMode unlockMode = standard);
    bool fUnlockRescanRequested;

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    SpectreClientBridge *clientBridge;
    QObject* qmlWebView;
    bool uiReady;

    QSharedPointer<ApplicationModelRemoteReplica> applicationModel; // holds reference to applicationmodel replica
    QSharedPointer<ClientModelRemoteReplica> clientModel; // holds reference to clientmodel replica
    QSharedPointer<WalletModelRemoteReplica> walletModel; // holds reference to applicationmodel replica
    QSharedPointer<AddressModelRemoteReplica> addressModel; // holds reference to addressmodel replica
    QSharedPointer<OptionsModelRemoteReplica> optionsModel; // holds reference to optionsmodel replica
//    ClientModel *clientModel;
//    WalletModel *walletModel;
//    MessageModel *messageModel;

    QMenuBar *appMenuBar;

    QSplashScreen *splashScreen;

    QAction *quitAction;
    QAction *aboutAction;
    QAction *optionsAction;
    QAction *toggleHideAction;
    QAction *exportAction;
    QAction *encryptWalletAction;
    QAction *backupWalletAction;
    QAction *changePassphraseAction;
    QAction *unlockWalletAction;
    QAction *lockWalletAction;
    QAction *aboutQtAction;
    QAction *openRPCConsoleAction;

    QSystemTrayIcon *trayIcon;
    Notificator *notificator;
    RPCConsole *rpcConsole;
    QTimer *pollTimer;

    uint64_t nWeight;
    bool fConnectionInit = true;

    /** Create the main UI actions. */
    void createActions();
    /** Create the menu bar and sub-menus. */
    void createMenuBar();
    /** Create system tray (notification) icon */
    void createTrayIcon();

    void execDialog(QDialog*const dialog);
    QProgressDialog* showProgressDlg(const QString &labelText);
    void closeProgressDlg(QProgressDialog *pDlg);

    bool initialized = false;

    friend class SpectreClientBridge;

    /**
     * @brief The WebElement class is written to provide easy access for modifying HTML objects with Javascript
     */
    class WebElement {
    public:
        enum SelectorType {ID,CLASS};
        WebElement(SpectreGUI* spectreGUI, QString name, SelectorType type = SelectorType::ID);
        void setAttribute(QString attribute, QString value);
        void removeAttribute(QString attribute);
        void addClass(QString className);
        void removeClass(QString className);
        void setContent(QString value);
    private:
        SpectreGUI* spectreGUI;
        QString name;
        QString getElementJS;
    };

public slots:
    /** Page finished loading and connection to core established */
    void pageLoaded();

    /** Set number of connections shown in the UI */
    void setNumConnections(int count);

private slots:
    /** Core sends message **/
    void updateCoreMessage(QString message);

    /** Handle external URLs **/
    void urlClicked(const QUrl & link);

    /** Set number of blocks shown in the UI */
    void setNumBlocks();
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void setEncryptionInfo(const EncryptionInfo& encryptionInfo);

    /** Notify the user of an error in the network or transaction handling code. */
    void error(const QString &title, const QString &message, bool modal);
    /** Asks the user whether to pay the transaction fee or to cancel the transaction.
       It is currently not possible to pass a return value to another thread through
       BlockingQueuedConnection, so an indirected pointer is used.
       https://bugreports.qt-project.org/browse/QTBUG-10440

      @param[in] nFeeRequired       the required fee
      @param[out] payFee            true to pay the fee, false to not pay the fee
    */
    void askFee(qint64 nFeeRequired, bool *payFee);
    void handleURI(QString strURI);

//#ifndef Q_OS_MACOS // commented because with QT 5.9.9 moc processor did not consider ifndef
    /** Handle tray icon clicked */
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
//#endif
    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void incomingTransaction(const QModelIndex & parent, int start, int end);

    /** Show configuration dialog */
    void optionsClicked();
    /** Show about dialog */
    void aboutClicked();

    /* Request to unlock for AXTO spent state determination, this slot should be called queued */
    void requestUnlockRescan();
    /** Unlock wallet */
    bool unlockWallet(UnlockMode unlockMode=UnlockMode::standard);
    /** Lock wallet */
    void lockWallet();
    /** Toggle whether wallet is locked or not */
    void toggleLock();
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();

    /** Show window if hidden, unminimize when minimized, rise when obscured or show if hidden and fToggleHidden is true */
    void showNormalIfMinimized(bool fToggleHidden = false);
    /** simply calls showNormalIfMinimized(true) for use in SLOT() macro */
    void toggleHidden();

    void updateStakingIcon(StakingInfo stakingInfo);

    /** called by a timer to check if fRequestShutdown has been set **/
    void detectShutdown();
    void requestShutdown();

    /** Stop application and delete all blockchain data. **/
    void resetBlockchain();
};

#endif
