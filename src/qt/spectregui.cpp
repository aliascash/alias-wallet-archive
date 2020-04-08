// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "spectregui.h"
#include "transactiontablemodel.h"
#include "transactionrecord.h"

#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "wallet.h"
#include "util.h"
#include "init.h"
#include "version.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QVBoxLayout>
#include <QIcon>
#include <QTimer>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QTextStream>
#include <QTextDocument>
#include <QDesktopWidget>
#include <QtWebView>
#include <QQuickWidget>
#include <QQuickItem>
#include <iostream>
#include <QNetworkProxy>

extern CWallet* pwalletMain;
double GetPoSKernelPS();
double GetPoSKernelPSRecent();

void SpectreGUI::runJavaScript(QString javascriptCode)
{
    if (uiReady)
        QMetaObject::invokeMethod(qmlWebView, "runJavaScript", Q_ARG(QString, javascriptCode));
}

SpectreGUI::WebElement::WebElement(SpectreGUI* spectreGUI, QString name, SelectorType type)
{
    this->spectreGUI = spectreGUI;
    this->name = name;
    switch (type) {
    case SelectorType::ID:
        this->getElementJS = "document.getElementById('" + this->name +"').";
        break;
    case SelectorType::CLASS:
        this->getElementJS = "document.getElementsByClassName('" + this->name +"')[0].";
        break;
    default:
        qFatal("SelectorType not reconized at WebElement::WebElement");
        break;
    }
}

void SpectreGUI::WebElement::setAttribute(QString attribute, QString value)
{
    //"setAttribute('data-title', '" + dataTitle + "');";
    value = value.replace("\n"," ");
    QString javascriptCode = getElementJS + "setAttribute(\"" + attribute + "\", \"" + value + "\");";
    spectreGUI->runJavaScript(javascriptCode);
}

void SpectreGUI::WebElement::removeAttribute(QString attribute)
{
    //"setAttribute('data-title', '" + dataTitle + "');";
    QString javascriptCode = getElementJS + "removeAttribute(\"" + attribute + "\");";
    spectreGUI->runJavaScript(javascriptCode);
}

void SpectreGUI::WebElement::addClass(QString className)
{
    QString javascriptCode = getElementJS + "classList.add(\""+className+"\");";
    spectreGUI->runJavaScript(javascriptCode);
}

void SpectreGUI::WebElement::removeClass(QString className)
{
    QString javascriptCode = getElementJS + "classList.remove(\""+className+"\");";
    spectreGUI->runJavaScript(javascriptCode);
}

SpectreGUI::SpectreGUI(QWebChannel *webChannel, QWidget *parent):
    QMainWindow(parent),
    bridge(new SpectreBridge(this)),
    clientModel(0),
    walletModel(0),
    messageModel(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    unlockWalletAction(0),
    lockWalletAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    nWeight(0),
    uiReady(false)
{
    // Make bridge available for JS client
    this->webChannel = webChannel;
    addJavascriptObjects(QStringLiteral("bridge"), bridge);

    resize(1280, 720);
    setWindowTitle(tr("Spectrecoin") + " - " + tr("Client") + " - " + tr(CLIENT_PLAIN_VERSION.c_str()));
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":icons/spectre"));
    setWindowIcon(QIcon(":icons/spectre"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    createMenuBar();

    rpcConsole = new RPCConsole(this);

    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(raise()));

    // prevents an oben debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
}

void initMessage(QSplashScreen *splashScreen, const std::string &message)
{
    if(splashScreen)
    {
        splashScreen->showMessage(QString::fromStdString("v"+FormatClientVersion()) + "\n" + QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(235,149,50));
        QApplication::instance()->processEvents();
    }
}

unsigned short const onion_port = 9089;

void SpectreGUI::loadIndex() {
#ifdef Q_OS_WIN
    QFile html("C:/spectrecoin-ui/index.html");
    QFileInfo webchannelJS("C:/spectrecoin-ui/qtwebchannel/qwebchannel.js");
#else
    QFile html("/opt/spectrecoin-ui/index.html");
    QFileInfo webchannelJS("/opt/spectrecoin-ui/qtwebchannel/qwebchannel.js");
#endif
    // Check if external qwebchannel exists and if not, create it! (this is how you get the right qwebchannel.js)
    if (html.exists() && !webchannelJS.exists()) {
        qDebug() << "Copy qwebchannel.js to" << webchannelJS.absoluteFilePath();
        QFile::copy(":/qtwebchannel/qwebchannel.js",webchannelJS.absoluteFilePath());
    }

    QQuickWidget *view = new QQuickWidget(this);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);    
    view->setSource(QUrl("qrc:///src/qt/res/main.qml"));
    qmlWebView = view->rootObject()->findChild<QObject*>("webView");
    QUrl url(html.exists() ? "file:///" + html.fileName() : "qrc:///src/qt/res/index.html");
    qmlWebView->setProperty("url", url);

    setCentralWidget(view);
    view->show();

//#ifdef TEST_TOR
//    QNetworkProxy proxy;
//    proxy.setType(QNetworkProxy::Socks5Proxy);
//    proxy.setHostName("127.0.0.1");
//    proxy.setPort(onion_port);
//    QNetworkProxy::setApplicationProxy(proxy);

//    //https://check.torproject.org
//    webEngineView->setUrl(QUrl("https://check.torproject.org"));
//#endif
}

SpectreGUI::~SpectreGUI()
{
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();

#ifdef Q_OS_MAC
    delete appMenuBar;
#endif
}

void SpectreGUI::pageLoaded(bool ok)
{
    uiReady = true;
    initMessage(splashScreen, "..Start UI..");

    // Create the tray icon (or setup the dock icon)
    if (!initialized) createTrayIcon();

    // Populate data
    walletModel->getOptionsModel()->emitDisplayUnitChanged(walletModel->getOptionsModel()->getDisplayUnit());
    walletModel->getOptionsModel()->emitReserveBalanceChanged(walletModel->getOptionsModel()->getReserveBalance());
    walletModel->getOptionsModel()->emitRowsPerPageChanged(walletModel->getOptionsModel()->getRowsPerPage());
    setNumConnections(clientModel->getNumConnections());
    setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
    setEncryptionStatus(walletModel->getEncryptionStatus());
    walletModel->emitEncryptionStatusChanged(walletModel->getEncryptionStatus());

    bridge->populateTransactionTable();
    bridge->populateAddressTable();

    initMessage(splashScreen, ".Start UI.");
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);
        walletModel->checkBalanceChanged(true);
        updateStakingIcon();
        if (GetBoolArg("-staking", true) && !initialized)
        {
            QTimer *timerStakingIcon = new QTimer(this);
            connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
            timerStakingIcon->start(5 * 1000);
        }
    }

    initMessage(splashScreen, "Ready!");
    if (splashScreen) splashScreen->finish(this);
    initialized = true;

    // If -min option passed, start window minimized.
    if(GetBoolArg("-min"))
        showMinimized();
    else
        show();

    pollTimer->start(MODEL_UPDATE_DELAY);
}

void SpectreGUI::addJavascriptObjects(const QString &id, QObject *object)
{
    //register a QObject to be exposed to JavaScript
    webChannel->registerObject(id, object);
}

void SpectreGUI::urlClicked(const QUrl & link)
{
    if(link.scheme() == "qrc" || link.scheme() == "file")
        return;

    QDesktopServices::openUrl(link);
}

void SpectreGUI::createActions()
{

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/spectre"), tr("&About Spectrecoin"), this);
    aboutAction->setToolTip(tr("Show information about Spectrecoin"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for Spectrecoin"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/spectre"), tr("&Show / Hide"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setToolTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(QIcon(":/icons/lock_open"), tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Lock Wallet"), this);
    lockWalletAction->setToolTip(tr("Lock wallet"));

    //exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    //exportAction->setToolTip(tr("Export the data in the current tab to a file"));
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));

    connect(quitAction, SIGNAL(triggered()), SLOT(requestShutdown()));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), SLOT(encryptWallet(bool)));
    connect(backupWalletAction, SIGNAL(triggered()), SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), SLOT(changePassphrase()));
    connect(unlockWalletAction, SIGNAL(triggered()), SLOT(unlockWallet()));
    connect(lockWalletAction, SIGNAL(triggered()), SLOT(lockWallet()));
}

void SpectreGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
    appMenuBar->hide();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(backupWalletAction);
    //file->addAction(exportAction);
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    settings->addAction(encryptWalletAction);
    settings->addAction(changePassphraseAction);
    settings->addAction(unlockWalletAction);
    settings->addAction(lockWalletAction);
    settings->addSeparator();
    settings->addAction(optionsAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void SpectreGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if (clientModel)
    {
        int mode = clientModel->getClientMode();
        if (mode != NT_FULL)
        {
            QString sMode = QString::fromLocal8Bit(GetNodeModeName(mode));
            if (sMode.length() > 0)
                sMode[0] = sMode[0].toUpper();

            setWindowTitle(tr("Spectrecoin") + " - " + tr("Wallet") + ", " + sMode);
        };

        // Replace some strings and icons, when using the testnet
        if (clientModel->isTestNet())
        {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(":icons/spectre_testnet"));
            setWindowIcon(QIcon(":icons/spectre_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/spectre_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(tr("Spectrecoin client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/spectre_testnet"));
                toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            }

            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }

        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));
        connect(clientModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));

        // Report errors from network/worker thread
        connect(clientModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        rpcConsole->setClientModel(clientModel);

        bridge->setClientModel();
    }
}

void SpectreGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if(walletModel)
    {
        // Report errors from wallet thread
        connect(walletModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),    SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock(WalletModel::UnlockMode)), this, SLOT(unlockWallet(WalletModel::UnlockMode)));

        connect(pollTimer, SIGNAL(timeout()), walletModel, SLOT(pollBalanceChanged()));

        bridge->setWalletModel();

        //register a QObject to be exposed to JavaScript
        addJavascriptObjects(QStringLiteral("walletModel"), walletModel);
        if (walletModel->getOptionsModel() != NULL)
            //register a QObject to be exposed to JavaScript
            addJavascriptObjects(QStringLiteral("optionsModel"), walletModel->getOptionsModel());
    }
}

void SpectreGUI::setSplashScreen(QSplashScreen * splashScreen)
{
    this->splashScreen = splashScreen;
}

void SpectreGUI::createTrayIcon()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("Spectrecoin client"));
    trayIcon->setIcon(QIcon(":/icons/spectre"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon, this);
}

//#ifndef Q_OS_MAC // commented because with QT 5.9.9 moc processor did not consider ifndef
void SpectreGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
//#endif

void SpectreGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void SpectreGUI::setNumConnections(int count)
{
    WebElement connectionIcon = WebElement(this, "connectionsIcon");

    QString className;

    switch(count)
    {
    case 0:          className = "connect-0"; break;
    case 1: case 2:  className = "connect-1"; break;
    case 3: case 4:  className = "connect-2"; break;
    case 5: case 6:  className = "connect-3"; break;
    case 7: case 8:  className = "connect-4"; break;
    case 9: case 10: className = "connect-5"; break;
    default:         className = "connect-6"; break;
    }

    connectionIcon.setAttribute("class", className);

    QString source = "qrc:///icons/" + className.replace("-", "_");
    connectionIcon.setAttribute("src", source);

    QString dataTitle = tr("%n active connection(s) to Spectrecoin network", "", count);
    connectionIcon.setAttribute("data-title", dataTitle);
}

void SpectreGUI::setNumBlocks(int count, int nTotalBlocks)
{
    WebElement blocksIcon = WebElement(this, "blocksIcon");
    WebElement syncingIcon = WebElement(this, "syncingIcon");
    WebElement syncProgressBar = WebElement(this, "syncProgressBar");

    // don't show / hide progress bar and its label if we have no connection to the network
    if (!clientModel || (clientModel->getNumConnections() == 0 && !clientModel->isImporting()))
    {
        syncProgressBar.setAttribute("style", "display:none;");
        return;
    }

    // -- translation (tr()) makes it difficult to neatly pick block/header
    static QString sBlockType = nNodeMode == NT_FULL ? tr("block") : tr("header");
    static QString sBlockTypeMulti = nNodeMode == NT_FULL ? tr("blocks") : tr("headers");

    QString strStatusBarWarnings = clientModel->getStatusBarWarnings();
    QString tooltip;

    if (nNodeMode != NT_FULL
        && nNodeState == NS_GET_FILTERED_BLOCKS)
    {
        tooltip = tr("Synchronizing with network...");
                + "<br>"
                + tr("Downloading filtered blocks...");

        int nRemainingBlocks = nTotalBlocks - pwalletMain->nLastFilteredHeight;
        float nPercentageDone = pwalletMain->nLastFilteredHeight / (nTotalBlocks * 0.01f);

        tooltip += "<br>"
                 + tr("~%1 filtered block(s) remaining (%2% done).").arg(nRemainingBlocks).arg(nPercentageDone);

        count = pwalletMain->nLastFilteredHeight;
        syncProgressBar.removeAttribute("style");
    } else
    if (count < nTotalBlocks)
    {
        int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);
        syncProgressBar.removeAttribute("style");

        if (strStatusBarWarnings.isEmpty())
        {
            bridge->networkAlert("");
            tooltip = clientModel->isImporting() ? tr("Importing blocks...") : tr("Synchronizing with network...");

            if (nNodeMode == NT_FULL)
            {
                tooltip += "<br>"
                         + tr("~%n block(s) remaining", "", nRemainingBlocks);
            } else
            {
                char temp[128];
                snprintf(temp, sizeof(temp), "~%%n %s remaining", nRemainingBlocks == 1 ? qPrintable(sBlockType) : qPrintable(sBlockTypeMulti));

                tooltip += "<br>"
                         + tr(temp, "", nRemainingBlocks);

            };
        }

        tooltip += (tooltip.isEmpty()? "" : "<br>")
         + (clientModel->isImporting() ? tr("Imported") : tr("Downloaded")) + " "
                 + tr("%1 of %2 %3 of transaction history (%4% done).").arg(count).arg(nTotalBlocks).arg(sBlockTypeMulti).arg(nPercentageDone, 0, 'f', 2);
    } else
    {
        tooltip = (clientModel->isImporting() ? tr("Imported") : tr("Downloaded")) + " " + tr("%1 blocks of transaction history.").arg(count);
    }

    // Override progressBarLabel text when we have warnings to display
    if (!strStatusBarWarnings.isEmpty())
        bridge->networkAlert(strStatusBarWarnings);

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    QString text;

    // Represent time from last generated block in human readable text
    if (secs <= 0)
    {
        // Fully up to date. Leave text empty.
    } else
    if (secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    } else
    if (secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    } else
    if (secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    } else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if (secs < 90*60 && count >= nTotalBlocks
        && nNodeState != NS_GET_FILTERED_BLOCKS)
    {
        tooltip = tr("Up to date") + "<br>" + tooltip;
        blocksIcon.removeClass("none");
        syncingIcon.addClass("none");

        //a js script to change the style property display to none for all outofsync elements
        QString javascript = "var divsToHide = document.getElementsByClassName('outofsync');";
                javascript+= "for(var i = 0; i < divsToHide.length; i++) {";
                javascript+= "     divsToHide[i].style.display = 'none';";
                javascript+= "}";

        runJavaScript(javascript);

        syncProgressBar.setAttribute("style", "display:none;");
    } else
    {
        tooltip = tr("Catching up...") + "<br>" + tooltip;

        blocksIcon.addClass("none");
        syncingIcon.removeClass("none");

        //a js script to change the style property display to inline for all outofsync elements
        QString javascript = "var divsToHide = document.getElementsByClassName('outofsync');";
                javascript+= "for(var i = 0; i < divsToHide.length; i++) {";
                javascript+= "     divsToHide[i].style.display = 'inline';";
                javascript+= "}";

        runJavaScript(javascript);

        syncProgressBar.removeAttribute("style");
    }

    if (!text.isEmpty())
    {
        tooltip += "<br>";
        tooltip += tr("Last received %1 was generated %2.").arg(sBlockType).arg(text);
    };

    blocksIcon.setAttribute("data-title", tooltip);
    syncingIcon.setAttribute("data-title", tooltip);
    syncProgressBar.setAttribute("data-title", tooltip);
    syncProgressBar.setAttribute("value", QString::number(count));
    syncProgressBar.setAttribute("max", QString::number(nTotalBlocks));
}

void SpectreGUI::error(const QString &title, const QString &message, bool modal)
{
    // Report errors from network/worker thread
    if(modal)
    {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else
    {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void SpectreGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void SpectreGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void SpectreGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage =
        tr("To process this transaction, a fee of %1 will be charged to support the network. "
           "Do you want to submit the transaction?").arg(
                BitcoinUnits::formatWithUnit(BitcoinUnits::XSPEC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void SpectreGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    qDebug() << "SpectreGUI::incomingTransaction";
    if(!walletModel || !clientModel || clientModel->inInitialBlockDownload() || (nNodeMode != NT_FULL && nNodeState != NS_READY))
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();

    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();

    // Ignore staking transactions... We should create an Option, and allow people to select/deselect what
    // type of transactions they want to see
    if(!(clientModel->getOptionsModel()->getNotifications().first() == "*")
    && ! clientModel->getOptionsModel()->getNotifications().contains(type))
        return;

    // On new transaction, make an info balloon
    // Unless the initial block download is in progress, to prevent balloon-spam
    QString date    = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    QString narration    = ttm->index(start, TransactionTableModel::Narration, parent).data().toString();
    QString address = ttm->index(start, TransactionTableModel::ToAddress, parent).data().toString();
    qint64 amount   = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QIcon   icon    = qvariant_cast<QIcon>(ttm->index(start, TransactionTableModel::ToAddress, parent).data(Qt::DecorationRole));

    notificator->notify(Notificator::Information,
                        tr("%1 %2")
                        .arg(BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                        .arg(type),
                        narration.size() > 0 ? tr("Address: %1\n" "Narration: %2\n").arg(address).arg(narration) :
                                               tr("Address: %1\n").arg(address),
                        icon);
}

void SpectreGUI::optionsClicked()
{
    bridge->triggerElement("#navitems a[href=#options]", "click");
    showNormalIfMinimized();
}

void SpectreGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void SpectreGUI::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void SpectreGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            handleURI(uri.toString());
            nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            bridge->triggerElement("#navitems a[href=#send]", "click");
        else
            notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Spectrecoin address or malformed URI parameters."));
    }

    event->acceptProposedAction();
}

void SpectreGUI::handleURI(QString strURI)
{
    SendCoinsRecipient rv;

    // URI has to be valid
    if(GUIUtil::parseBitcoinURI(strURI, &rv))
    {
        CBitcoinAddress address(rv.address.toStdString());
        if (!address.IsValid())
            return;

        bridge->emitReceipient(rv.address, rv.label, rv.narration, rv.amount);

        showNormalIfMinimized();
    }
    else
        notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Spectrecoin address or malformed URI parameters."));
}

void SpectreGUI::setEncryptionStatus(int status)
{
    WebElement encryptionIcon    = WebElement(this, "encryptionIcon");
    WebElement encryptButton     = WebElement(this, "encryptWallet");
    WebElement encryptMenuItem   = WebElement(this, "encryptWallet", WebElement::SelectorType::CLASS);
    WebElement changePassphrase  = WebElement(this, "changePassphrase");
    WebElement toggleLock        = WebElement(this, "toggleLock");
    WebElement toggleLockIcon    = WebElement(this, "toggleLock i");
    switch(status)
    {
    case WalletModel::Unencrypted:
        encryptionIcon.setAttribute("style", "display:none;");
        changePassphrase.addClass("none");
        toggleLock.addClass("none");
        encryptMenuItem.removeClass("none");
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        encryptMenuItem  .addClass("none");
        encryptionIcon.removeAttribute("style");
        encryptionIcon.removeClass("fa-lock");
        encryptionIcon.removeClass("encryption");
        encryptionIcon.   addClass("fa-unlock");
        encryptionIcon.   addClass("no-encryption");
        encryptMenuItem  .addClass("none");
        toggleLockIcon.removeClass("fa-unlock");
        toggleLockIcon.removeClass("fa-unlock-alt");
        toggleLockIcon.   addClass("fa-lock");
        encryptionIcon   .setAttribute("src", "qrc:///icons/lock_open");

        if (fWalletUnlockStakingOnly)
        {
            encryptionIcon   .setAttribute("data-title", tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for staking only"));
            encryptionIcon.removeClass("red");
            encryptionIcon.addClass("orange");
            encryptionIcon.addClass("encryption-stake");

            toggleLockIcon  .removeClass("red");
            toggleLockIcon     .addClass("orange");
        } else
        {
            encryptionIcon   .setAttribute("data-title", tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
            encryptionIcon.addClass("red");
            encryptionIcon.removeClass("orange");
            encryptionIcon.removeClass("encryption-stake");

            toggleLockIcon  .removeClass("orange");
            toggleLockIcon     .addClass("red");
        };

        encryptButton.addClass("none");
        changePassphrase.removeClass("none");
        toggleLock.removeClass("none");
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        encryptionIcon.removeAttribute("style");
        encryptionIcon.removeClass("fa-unlock");
        encryptionIcon.removeClass("no-encryption");
        encryptionIcon.removeClass("encryption-stake");
        encryptionIcon.   addClass("fa-lock");
        encryptionIcon.   addClass("encryption");
        toggleLockIcon.removeClass("fa-lock");
        toggleLockIcon.   addClass("fa-unlock-alt");
        encryptionIcon   .setAttribute("data-title", tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));

        encryptionIcon     .addClass("red");
        encryptionIcon  .removeClass("orange");
        encryptButton      .addClass("none");
        encryptMenuItem    .addClass("none");
        changePassphrase.removeClass("none");
        toggleLockIcon  .removeClass("orange");
        toggleLockIcon     .addClass("red");
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void SpectreGUI::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt:
                                     AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void SpectreGUI::backupWallet()
{
    if (QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).count() == 0) {
        qFatal("QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).count() == 0");
    }
    QString saveDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));
    if(!filename.isEmpty())
    {
        if(!walletModel->backupWallet(filename))
        {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the wallet data to the new location."));
        }
    }
}

void SpectreGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void SpectreGUI::unlockWallet(WalletModel::UnlockMode unlockMode)
{
    if(!walletModel)
        return;

    AskPassphraseDialog::Mode mode;
    if (unlockMode == WalletModel::UnlockMode::rescan) {
         mode = AskPassphraseDialog::UnlockRescan;
    }
    else {
        mode = sender() == unlockWalletAction ?
                    AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
    }

    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(mode, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void SpectreGUI::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void SpectreGUI::toggleLock()
{
    if(!walletModel)
        return;
    WalletModel::EncryptionStatus status = walletModel->getEncryptionStatus();

    switch(status)
    {
        case WalletModel::Locked:       unlockWalletAction->trigger(); break;
        case WalletModel::Unlocked:     lockWalletAction->trigger();   break;
        default: // unencrypted wallet
            QMessageBox::warning(this, tr("Lock Wallet"),
                tr("Error: Wallet must first be encrypted to be locked."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
    };
}

void SpectreGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void SpectreGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void SpectreGUI::updateWeight()
{
    if (!pwalletMain)
        return;

    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;

    TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
    if (!lockWallet)
        return;

    nWeight = pwalletMain->GetStakeWeight() + pwalletMain->GetSpectreStakeWeight();
}

void SpectreGUI::updateStakingIcon()
{
    WebElement stakingIcon = WebElement(this, "stakingIcon");
    uint64_t nNetworkWeight = 0, nNetworkWeightRecent;

    if(fIsStaking)
    {
        updateWeight();
        nNetworkWeight = GetPoSKernelPS();
        nNetworkWeightRecent = GetPoSKernelPSRecent();
    } else
        nWeight = 0;

    if (fIsStaking && nWeight)
    {
        uint64_t nWeight = this->nWeight;

        unsigned nEstimateTime = GetTargetSpacing(nBestHeight, GetAdjustedTime()) * nNetworkWeight / nWeight;
        QString text, textDebug;

        text = (nEstimateTime < 60)           ? tr("%1 second(s)").arg(nEstimateTime) : \
               (nEstimateTime < 60 * 60)      ? tr("%1 minute(s), %2 second(s)").arg(nEstimateTime / 60).arg(nEstimateTime % 60) : \
               (nEstimateTime < 24 * 60 * 60) ? tr("%1 hour(s), %2 minute(s)").arg(nEstimateTime / (60 * 60)).arg((nEstimateTime % (60 * 60)) / 60) : \
                                                tr("%1 day(s), %2 hour(s)").arg(nEstimateTime / (60 * 60 * 24)).arg((nEstimateTime % (60 * 60 * 24)) / (60 * 60));

        stakingIcon.removeClass("not-staking");
        stakingIcon.   addClass("staking");
        //stakingIcon.   addClass("fa-spin"); // TODO: Replace with gif... too much cpu usage

        nWeight        /= COIN;
        nNetworkWeight /= COIN;
        nNetworkWeightRecent /= COIN;

        if (fDebug)
            textDebug = tr(" (last 72 blocks %1)").arg(nNetworkWeightRecent);

        stakingIcon.setAttribute("data-title", tr("Staking.<br/>Your weight is %1<br/>Network weight is %2%3<br/>Expected time to earn reward is %4").arg(nWeight).arg(nNetworkWeight).arg(textDebug).arg(text));
    } else
    {
        stakingIcon.addClass("not-staking");
        stakingIcon.removeClass("staking");
        //stakingIcon.removeClass("fa-spin"); // TODO: See above TODO...

        stakingIcon.setAttribute("data-title", (nNodeMode == NT_THIN)                   ? tr("Not staking because wallet is in thin mode") : \
                                               (!GetBoolArg("-staking", true))          ? tr("Not staking, staking is disabled")  : \
                                               (pwalletMain && pwalletMain->IsLocked()) ? tr("Not staking because wallet is locked")  : \
                                               (vNodes.empty())                         ? tr("Not staking because wallet is offline") : \
                                               (clientModel->inInitialBlockDownload())  ? tr("Not staking because wallet is syncing") : \
                                               (!fIsStaking)                            ? tr("Initializing staking...") : \
                                               (!nWeight)                               ? tr("Not staking because you don't have mature coins") : \
                                                                                          tr("Not staking"));
    }
}

void SpectreGUI::requestShutdown()
{
    StartShutdown();
}

void SpectreGUI::detectShutdown()
{
    if (ShutdownRequested())
        QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}
