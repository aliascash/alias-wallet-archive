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

#include "spectrebridge.h"
#include "walletmodel.h"
#include "askpassphrasedialog.h"
#include "rpcconsole.h"

#include <stdint.h>

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
QT_END_NAMESPACE

static const int WEBSOCKETPORT = 52471;
static const int WEBSOCKETPORT_TESTNET = 52472;

/**
  Spectre GUI main class. This class represents the main window of the Spectre UI. It communicates with both the client and
  wallet models to give the user an up-to-date view of the current core state.
*/
class SpectreGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit SpectreGUI(QWebChannel *webChannel, QWidget *parent = 0);
    ~SpectreGUI();

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel *walletModel);
    /** Set the message model.
        The message model represents encryption message database, and offers access to the list of messages, address book and sending
        functionality.
    */
    void setMessageModel(MessageModel *messageModel);

    void setSplashScreen(QSplashScreen* splash);

    void loadIndex();

    void runJavaScript(QString javascriptCode);

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    SpectreBridge *bridge;
    QWebChannel * webChannel;
    QObject* qmlWebView;
    bool uiReady;

    ClientModel *clientModel;
    WalletModel *walletModel;
    MessageModel *messageModel;

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

    /** Page finished loading and connection to core established */
    void pageLoaded(bool ok);
    bool initialized = false;

    friend class SpectreBridge;

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


private slots:
    /** Add JavaScript objects to page */
    void addJavascriptObjects(const QString &id, QObject *object);
    /** Handle external URLs **/
    void urlClicked(const QUrl & link);

    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void setNumBlocks(int count, int nTotalBlocks);
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void setEncryptionStatus(int status);

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

    /** Unlock wallet */
    void unlockWallet(WalletModel::UnlockMode unlockMode=WalletModel::UnlockMode::standard);
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

    void updateWeight();
    void updateStakingIcon();

    /** called by a timer to check if fRequestShutdown has been set **/
    void detectShutdown();
    void requestShutdown();
};

#endif
