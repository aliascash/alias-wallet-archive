// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
//
// SPDX-License-Identifier: MIT

#include "applicationmodel.h"
#include "spectrebridge.h"
#include "spectregui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "paymentserver.h"
#include "winshutdownmonitor.h"
#include "setupwalletwizard.h"
#include "bootstrapwizard.h"

#include "init.h"
#include "interface.h"
#include "net.h" // TODO for torPath variable, extract functionality for tor in separate header

#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>
#include <QLibraryInfo>
#include <QTimer>

#include <QWebChannel>
#include <QWebSocketServer>
#include <QUuid>

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

#ifndef WIN32
#include <signal.h>
#endif

namespace fs = boost::filesystem;

// Need a global reference for the notifications to find the GUI
static SpectreGUI *guiref;
static QSplashScreen *splashref;
static ApplicationModel *applicationModelRef;
static PaymentServer *paymentServiceRef;
static BootstrapWizard *bootstrapWizard;

static int evaluate(QRemoteObjectPendingReply<int> pendingReply) {
    if (pendingReply.waitForFinished())
        return pendingReply.returnValue();
    else {
        return 0;
    }
}

static void ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style)
{
    // Message from network thread
    if(guiref)
    {
        if (splashref)
            splashref->finish(guiref);

        bool modal = (style & CClientUIInterface::MODAL);
        // in case of modal message, use blocking connection to wait for user to click OK
        QMetaObject::invokeMethod(guiref, "error",
                                   modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                                   Q_ARG(QString, QString::fromStdString(caption)),
                                   Q_ARG(QString, QString::fromStdString(message)),
                                   Q_ARG(bool, modal));
    } else
    {
        LogPrintf("%s: %s\n", caption.c_str(), message.c_str());
        fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
    }
}

static bool ThreadSafeAskFee(int64_t nFeeRequired, const std::string& strCaption)
{
    if(!guiref)
        return false;
    if(nFeeRequired < nMinTxFee || nFeeRequired <= nTransactionFee || fDaemon)
        return true;
    bool payFee = false;

    QMetaObject::invokeMethod(guiref, "askFee", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(qint64, nFeeRequired),
                               Q_ARG(bool*, &payFee));
    return payFee;
}

static void ThreadSafeHandleURI(const std::string& strURI)
{
    if(!guiref)
        return;

    QMetaObject::invokeMethod(guiref, "handleURI", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(QString, QString::fromStdString(strURI)));
}

static void InitMessage(const std::string &message)
{
    if (fDebug) LogPrintf("%s\n", message);
    if (applicationModelRef)
    {
        if (coreRunningMode == CoreRunningMode::RUNNING_NORMAL)
            applicationModelRef->setCoreMessage(QString::fromStdString(message));
#ifdef ANDROID
        QtAndroid::androidService().callMethod<void>("updateNotification", "(Ljava/lang/String;Ljava/lang/String;I)V",
                                                     QAndroidJniObject::fromString(QObject::tr("Initializing")).object<jstring>(),
                                                     QAndroidJniObject::fromString(QString::fromStdString(message)).object<jstring>(),
                                                     1);
#endif
    }
    if(splashref) {
        splashref->showMessage(QString::fromStdString("v" + FormatClientVersion() + "\n" + message + "\n\n"), Qt::AlignBottom|Qt::AlignHCenter, QColor(138,140,142));
    }
    if (splashref || applicationModelRef)
        QApplication::instance()->processEvents();
}

static void InitQMessage(const QString &message)
{
    if(splashref)
    {
        splashref->showMessage(QString::fromStdString("v"+FormatClientVersion()) + "\n" + message + "\n\n", Qt::AlignBottom|Qt::AlignHCenter, QColor(138,140,142));
        QApplication::instance()->processEvents();
    }
}

/*
   Translate string to current locale using Qt.
 */
static std::string Translate(const char* psz)
{
    return QCoreApplication::translate("bitcoin-core", psz).toStdString();
}

/* Handle runaway exceptions. Shows a message box with the problem and quits the program.
 */
static void handleRunawayException(std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    QMessageBox::critical(0, "Runaway exception", SpectreGUI::tr("A fatal error occurred. Alias can no longer continue safely and will quit.") + QString("\n\n") + QString::fromStdString(strMiscWarning));
    exit(1);
}

void ThreadCoreRunningModeHandler(void *nothing)
{
    RenameThread("coreRunningMode");

    while (!ShutdownRequested())
    {
        if (coreRunningMode != CoreRunningMode::RUNNING_NORMAL && coreRunningMode != CoreRunningMode::UI_PAUSED)
        {
            LOCK(cs_main);
            bool staking = !pwalletMain->IsLocked() && fIsStakingEnabled;
            if (!ShutdownRequested() && !staking && vNodes.size() > 2 && !IsInitialBlockDownload() &&
                    nBestHeight >= GetNumBlocksOfPeers() && pindexBest->GetBlockTime() > (GetTime() - 15 * 60))
            {
                if (fDebug) LogPrintf("ThreadCoreRunningModeHandler service >> sleep\n");

                // lock wallet to make sure there is no ongoing wallet operation
                // LogPrintf("ThreadCoreRunningModeHandler service >> sleep: lock wallet\n");
                ENTER_CRITICAL_SECTION(pwalletMain->cs_wallet);

                // net locks, to make sure no more network operations are done
                // LogPrintf("ThreadCoreRunningModeHandler service >> sleep: lock cs_vAddedNodes\n");
                ENTER_CRITICAL_SECTION(cs_vAddedNodes); // blocks ThreadOpenAddedConnections
                // LogPrintf("ThreadCoreRunningModeHandler service >> sleep: lock cs_connectNode\n");
                ENTER_CRITICAL_SECTION(cs_connectNode); // blocks ThreadOpenConnections
                // LogPrintf("ThreadCoreRunningModeHandler service >> sleep: lock cs_vNodes\n");
                ENTER_CRITICAL_SECTION(cs_vNodes); // blocks ThreadSocketHandler

                // Disconnect all nodes
                BOOST_FOREACH(CNode* pnode, vNodes)
                    pnode->fDisconnect = true;
                // LogPrintf("ThreadCoreRunningModeHandler service >> sleep: disconnect nodes\n");
                ThreadSocketHandler_DisconnectNodes();
                // LogPrintf("ThreadCoreRunningModeHandler service >> sleep: NotifyNumConnectionsChanged %d\n", vNodes.size());
                uiInterface.NotifyNumConnectionsChanged(vNodes.size());
                QMetaObject::invokeMethod(applicationModelRef, "updateCoreSleeping", Qt::QueuedConnection, Q_ARG(bool, true));

                if (coreRunningMode == CoreRunningMode::REQUEST_SYNC_SLEEP)
                    coreRunningMode = CoreRunningMode::SLEEP;

                while (coreRunningMode == CoreRunningMode::SLEEP && !ShutdownRequested())
                {
                    MilliSleep(100);
                }

                if (fDebug) LogPrintf("ThreadCoreRunningModeHandler service >> resume\n");
                LEAVE_CRITICAL_SECTION(cs_vNodes);
                LEAVE_CRITICAL_SECTION(cs_connectNode);
                LEAVE_CRITICAL_SECTION(cs_vAddedNodes);
                LEAVE_CRITICAL_SECTION(pwalletMain->cs_wallet);

                QMetaObject::invokeMethod(applicationModelRef, "updateCoreSleeping", Qt::QueuedConnection, Q_ARG(bool, false));
            }
        }
        if (!ShutdownRequested())
            MilliSleep(1000);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AndroidAppInit(int argc, char* argv[])
{
    QAndroidService app(argc, argv);
    qInfo() << "Android service starting...";

    uiInterface.InitMessage.connect(InitMessage);

    boost::thread_group threadGroup;

    // Android: detect location of tor binary
    QString nativeLibraryDir = QAndroidJniObject::getStaticObjectField<jstring>("org/alias/wallet/AliasService","nativeLibraryDir").toString();
    torPath = nativeLibraryDir.toStdString() + "/libtor.so";

    bool fRet = false;
    try
    {
        //
        // Parameters
        //
        ParseParameters(argc, argv);
        ReadConfigFile(mapArgs, mapMultiArgs);
        SoftSetBoolArg("-debug", true);

        // ... then GUI settings:
        OptionsModel optionsModel;

        bool rescan = false;
        bool init = QtAndroid::androidService().getField<jboolean>("init");
        while(!init)
        {   // Wait for intent extras set
            QApplication::instance()->processEvents(QEventLoop::AllEvents, 100);
            init = QtAndroid::androidService().getField<jboolean>("init");
        }
        rescan = QtAndroid::androidService().getField<jboolean>("rescan");
        if (rescan) SoftSetBoolArg("-rescan", true);

        //---- Create core webSocket server for JavaScript client
        QWebSocketServer server(QStringLiteral("Alias Core Websocket Server"), QWebSocketServer::NonSecureMode);
        if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET : WEBSOCKETPORT))
            throw std::runtime_error(strprintf("Alias Core QWebSocketServer failed to listen on port %d", fTestNet ? WEBSOCKETPORT_TESTNET : WEBSOCKETPORT));
        qDebug() << "Core QWebSocketServer started: " << server.serverAddress() << ":" << server.serverPort();
        QString webSocketToken = QUuid::createUuid().toString().remove(QChar('{')).remove(QChar('}'));
        // qDebug() << "QWebSocketServer access token: " << webSocketToken;
        WebSocketClientWrapper clientWrapper(&server, webSocketToken);  // wrap WebSocket clients in QWebChannelAbstractTransport objects
        QWebChannel webChannel;                         // setup the channel
        QObject::connect(&clientWrapper, &WebSocketClientWrapper::clientConnected, &webChannel, &QWebChannel::connectTo);

        //---- Setup remote objects host
        QRemoteObjectHost srcNode(QUrl(QStringLiteral("local:alias")));
        ApplicationModel applicationModel;
        applicationModelRef = &applicationModel;
        applicationModel.setWebSocketToken(webSocketToken);
        srcNode.enableRemoting(&applicationModel);  // enable remoting
        srcNode.enableRemoting(&optionsModel);      // enable remoting

        // New Thread for controlling the sleep mode
        NewThread(&ThreadCoreRunningModeHandler, NULL);

        // Initialize Core
        fRet = AppInit2(threadGroup);

        if (fRet)
        {
            // reset androidservice startup flags
            QtAndroid::androidService().setField<jboolean>("rescan", false);

            // Get locks upfront, to make sure we can completly setup our client before core sends notifications
            ENTER_CRITICAL_SECTION(cs_main); // no RAII
            ENTER_CRITICAL_SECTION(pwalletMain->cs_wallet); // no RAII

            // create models
            WalletModel walletModel(&applicationModel, pwalletMain, &optionsModel);
            InitMessage("Init client models...");
            ClientModel clientModel(&applicationModel, &optionsModel, &walletModel);
            SpectreBridge bridge(&webChannel);
            bridge.setClientModel(&clientModel);
            bridge.setWalletModel(&walletModel);
            bridge.setApplicationModel(&applicationModel);

            // Manually create a blockChangedEvent to set initial values for the UI
            BlockChangedEvent blockChangedEvent = { nBestHeight, GetNumBlocksOfPeers(), IsInitialBlockDownload(), nNodeMode == NT_FULL ?
                                                    pindexBest ? pindexBest->GetBlockTime() : GENESIS_BLOCK_TIME :
                                                    pindexBestHeader ? pindexBestHeader->GetBlockTime() : GENESIS_BLOCK_TIME };
            clientModel.updateNumBlocks(blockChangedEvent);

            // Register remote objects
            srcNode.enableRemoting(&clientModel);
            srcNode.enableRemoting(&walletModel);
            srcNode.enableRemoting(bridge.getAddressModel());

            // Release lock before starting event processing, otherwise lock would never be released
            LEAVE_CRITICAL_SECTION(pwalletMain->cs_wallet);
            LEAVE_CRITICAL_SECTION(cs_main);

            app.exec();

            LogPrintf("Core shutdown.\n\n");
            srcNode.disableRemoting(&applicationModel);
            threadGroup.interrupt_all();
            threadGroup.join_all();
        }
        else
        {
            LogPrintf("Init not successfull: Core shutdown.\n\n");
            threadGroup.interrupt_all();
            // threadGroup.join_all(); was left out intentionally here, because we didn't re-test all of
            // the startup-failure cases to make sure they don't result in a hang due to some
            // thread-blocking-waiting-for-another-thread-during-startup case
        }
        Shutdown();
        applicationModelRef->AfterShutdown();
        applicationModelRef = 0;
    } catch (std::exception& e) {
        PrintException(&e, "AndroidService");
        fRet = false;
    } catch (...) {
        PrintException(NULL, "AndroidService");
        fRet = false;
    };

    qInfo() << "Android service stopped.";
    return fRet;
}

static void RemoteModelStateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState)
{
    switch(state)
    {
    case QRemoteObjectReplica::Suspect:
        StartShutdown();
        QtAndroid::androidActivity().callMethod<void>("finishAndRemoveTask");
        break;
    default:
        break;
    }
}


#ifndef SPECTRE_QT_TEST
int main(int argc, char *argv[])
{
    qDebug() << "App start in main.cpp";
#ifndef WIN32
    // Block signals. We handle them in a thread.
    if (BlockSignals() != 0)
        return 1;
#else
    // Hide the console for windows
    FreeConsole();
#endif

    fHaveGUI = true;

    // Command-line options take precedence:
    ParseParameters(argc, argv);

    if (GetBoolArg("-service", false))
    {
        bool fRet = AndroidAppInit(argc, argv);
        return (fRet ? 0 : 1);
    }

#if QT_VERSION < 0x050000
    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    // Make sure TESTNET flag and specific chainparams are set (Init in AppInit2 is to late)
    fTestNet = GetBoolArg("-testnet", false);
    if (!SelectParamsFromCommandLine())
    {
        QMessageBox::critical(0, "Alias", QString("Error: Invalid combination of -testnet and -regtest."));
        return 1;
    }

    QApplication app(argc, argv);

    // Set global styles
    app.setStyleSheet("a {color: #f28321; }");
    QPalette newPal(app.palette());
    newPal.setColor(QPalette::Link, QColor(242, 131, 33));
    newPal.setColor(QPalette::LinkVisited, QColor(242, 131, 33));
    app.setPalette(newPal);

    QtWebView::initialize();

    // Do this early as we don't want to bother initializing if we are just calling IPC
    // ... but do it after creating app, so QCoreApplication::arguments is initialized:
    if (PaymentServer::ipcSendCommandLine())
        exit(0);
    PaymentServer* paymentServer = new PaymentServer(&app);
    paymentServiceRef = paymentServer;

    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));

    #if defined(Q_OS_WIN)
        // Install global event filter for processing Windows session related Windows messages (WM_QUERYENDSESSION and WM_ENDSESSION)
        qApp->installNativeEventFilter(new WinShutdownMonitor());
    #endif

    // ... then alias.conf:
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        // This message can not be translated, as translation is not initialized yet
        // (which not yet possible because lang=XX can be overridden in bitcoin.conf in the data directory)
        QMessageBox::critical(0, "Alias",
                              QString("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(mapArgs["-datadir"])));
        return 1;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);

    // Application identification (must be set before OptionsModel is initialized,
    // as it is used to locate QSettings)
    app.setOrganizationName("The Alias Foundation");
    app.setOrganizationDomain("alias.cash");
    if(GetBoolArg("-testnet")) // Separate UI settings for testnet
        app.setApplicationName("Alias-testnet");
    else
        app.setApplicationName("Alias");

    // ... then GUI settings:
    OptionsModel optionsModel;

    // Get desired locale (e.g. "de_DE") from command line or use system locale
    QString lang_territory = QString::fromStdString(GetArg("-lang", QLocale::system().name().toStdString()));
    QString lang = lang_territory;
    // Convert to "de" only by truncating "_DE"
    lang.truncate(lang_territory.lastIndexOf('_'));

    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;
    // Load language files for configured locale:
    // - First load the translator for the base language, without territory
    // - Then load the more specific locale translator

    // Load e.g. qt_de.qm
    if (qtTranslatorBase.load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslatorBase);

    // Load e.g. qt_de_DE.qm
    if (qtTranslator.load("qt_" + lang_territory, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslator);

    // Load e.g. bitcoin_de.qm (shortcut "de" needs to be defined in bitcoin.qrc)
    if (translatorBase.load(lang, ":/translations/"))
        app.installTranslator(&translatorBase);

    // Load e.g. bitcoin_de_DE.qm (shortcut "de_DE" needs to be defined in bitcoin.qrc)
    if (translator.load(lang_territory, ":/translations/"))
        app.installTranslator(&translator);

    // Subscribe to global signals from core
    uiInterface.ThreadSafeMessageBox.connect(ThreadSafeMessageBox);
    //uiInterface.ThreadSafeAskFee.connect(ThreadSafeAskFee);
    uiInterface.ThreadSafeHandleURI.connect(ThreadSafeHandleURI);
    //TODO obsolete uiInterface.InitMessage.connect(InitMessage);
    //uiInterface.QueueShutdown.connect(QueueShutdown);
    uiInterface.Translate.connect(Translate);

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        GUIUtil::HelpMessageBox help;
        help.showOrPrint();
        return 1;
    }

#ifdef ANDROID
    bool blkDatExists = fs::exists(GetDataDir() / "blk0001.dat");
    bool txleveldbExists = fs::exists(GetDataDir() / "txleveldb");
    // Show bootstrap wizard if blockchain was not updated since 14 days.
    double blkDataOldSeconds = 14 * 24 * 60 * 60;
    bool blockchainStale = blkDatExists && std::difftime(std::time(0), fs::last_write_time(GetDataDir() / "blk0001.dat")) > blkDataOldSeconds;
    if (!blkDatExists || !txleveldbExists || blockchainStale)
    {
        BootstrapWizard wizard(blockchainStale ? 14 : 0);
        bootstrapWizard = &wizard;
        wizard.show();
        if (!wizard.exec())
            return 0;
    }
    fs::remove_all(GetDataDir() / "tmp_bootstrap");
    bootstrapWizard = nullptr;
#endif

    // Start SetupWalletWizard if no wallet.dat exists
    if (!mapArgs.count("-bip44key") && !mapArgs.count("-wallet") && !mapArgs.count("-salvagewallet") && !fs::exists(GetDataDir() / "wallet.dat"))
    {
        SetupWalletWizard wizard;
        wizard.show();
        if (!wizard.exec())
            return 0;
        if (wizard.hasVisitedPage(SetupWalletWizard::Page_RecoverFromMnemonic))
            SoftSetBoolArg("-rescan", true);
    }

#ifdef ANDROID
    // For Android, adjust width of splash screen to fill width.
    QRect  screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();

    int height = screenGeometry.width() < screenGeometry.height() ?
                 screenGeometry.width() - (screenGeometry.width() / 5) :
                 screenGeometry.height() / 2;
    int width = screenGeometry.width() < screenGeometry.height() ? height : height / 4 * 10;
    QRect targetRect((screenGeometry.width() - width) / 2,
                     screenGeometry.width() < screenGeometry.height() ?
                         ((screenGeometry.height() - height) / 7 * 3) :
                         ((screenGeometry.height() - height) / 5 * 2),
                     width, height);

    QPixmap splashPixmap(GUIUtil::createPixmap(screenGeometry.width(), screenGeometry.height(), QColor(40, 40, 41),
                                               screenGeometry.width() < screenGeometry.height() ? QString(":/assets/svg/Alias-Stacked-Reverse.svg") : QString(":/assets/svg/Alias-Horizontal-Reverse.svg"),
                                               targetRect));

    // Start core service (if service is allready running, this has not effect.)
    std::string bip44key = GetArg("-bip44key", "");
    QtAndroid::androidActivity().callMethod<void>("startCore", "(ZLjava/lang/String;)V", GetBoolArg("-rescan"), QAndroidJniObject::fromString(QString::fromStdString(bip44key)).object<jstring>());
#else
    QPixmap splashPixmap(GUIUtil::createPixmap(600, 686, QColor(40, 40, 41), QString(":/assets/svg/Alias-Stacked-Reverse.svg"), QRect(62, 87, 476, 476)));
#endif
    QSplashScreen splash(splashPixmap, 0);
    if (GetBoolArg("-splash", true) && !GetBoolArg("-min"))
    {
        splash.setEnabled(false);
#ifdef ANDROID
        splash.showFullScreen();
#else
        splash.show();
#endif
        splashref = &splash;
    }

    app.processEvents();

    app.setQuitOnLastWindowClosed(false);

    try
    {
        // Regenerate startup link, to fix links to old versions
        if (GUIUtil::GetStartOnSystemStartup())
            GUIUtil::SetStartOnSystemStartup(true);

#ifndef ANDROID
        boost::thread_group threadGroup;
#endif
        InitMessage("Connect service...");

        // Accuire remote objects replicas
        QRemoteObjectNode repNode;
        QSharedPointer<ClientModelRemoteReplica> clientModelPtr; // holds reference to clientmodel replica
        QSharedPointer<ApplicationModelRemoteReplica> applicationModelPtr; // holds reference to applicationmodel replica
        QSharedPointer<WalletModelRemoteReplica> walletModelPtr; // holds reference to walletmodel replica
        QSharedPointer<AddressModelRemoteReplica> addressModelPtr; // holds reference to walletmodel replica
        QSharedPointer<OptionsModelRemoteReplica> optionsModelPtr; // holds reference to optionsmodel replica
        repNode.connectToNode(QUrl(QStringLiteral("local:alias"))); // connect with remote host node

        applicationModelPtr.reset(repNode.acquire<ApplicationModelRemoteReplica>()); // acquire replica of source from host node
        clientModelPtr.reset(repNode.acquire<ClientModelRemoteReplica>()); // acquire replica of source from host node
        walletModelPtr.reset(repNode.acquire<WalletModelRemoteReplica>()); // acquire replica of source from host node
        addressModelPtr.reset(repNode.acquire<AddressModelRemoteReplica>());
        optionsModelPtr.reset(repNode.acquire<OptionsModelRemoteReplica>()); // acquire replica of source from host node

        QObject::connect(applicationModelPtr.data(), &ApplicationModelRemoteReplica::coreMessageChanged, InitQMessage);
        QObject::connect(applicationModelPtr.data(), &ApplicationModelRemoteReplica::stateChanged, RemoteModelStateChanged);

        if (!applicationModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : ApplicationModelRemoteReplica was not initialized!");
        if (!optionsModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : OptionsModelRemoteReplica was not initialized!");
        if (!clientModelPtr->waitForSource(-1))
            throw std::runtime_error("SpectreGUI() : ClientModelRemoteReplica was not initialized!");
        if (!walletModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : WalletModelRemoteReplica was not initialized!");
        if (!addressModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : AddressModelRemoteReplica was not initialized!");

        //---- Create webSocket server for JavaScript client
        QWebSocketServer server(QStringLiteral("Alias Client Websocket Server"), QWebSocketServer::NonSecureMode);
        if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET + 1 : WEBSOCKETPORT + 1)) {
            if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET + 1: WEBSOCKETPORT + 1)) {
                qFatal("Alias Client QWebSocketServer failed to listen on port %d", fTestNet ? WEBSOCKETPORT_TESTNET + 1: WEBSOCKETPORT + 1);
                return 1;
            }
        }
        qDebug() << "Client QWebSocketServer started: " << server.serverAddress() << ":" << server.serverPort();
        QString webSocketToken = QUuid::createUuid().toString().remove(QChar('{')).remove(QChar('}'));
        // qDebug() << "QWebSocketServer access token: " << webSocketToken;
        WebSocketClientWrapper clientWrapper(&server, applicationModelPtr.data()->webSocketToken()); // wrap WebSocket clients in QWebChannelAbstractTransport objects
        QWebChannel webChannel;                                                                      // setup the channel
        QObject::connect(&clientWrapper, &WebSocketClientWrapper::clientConnected, &webChannel, &QWebChannel::connectTo);

        SpectreGUI window(applicationModelPtr, clientModelPtr, walletModelPtr, addressModelPtr, optionsModelPtr);
        window.setSplashScreen(&splash);
        guiref = &window;

        SpectreClientBridge clientBridge(&window, &webChannel);

        // Periodically check if shutdown was requested to properly quit the Qt application
        #if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("Alias Core did't yet exit safely..."), (HWND)window.winId());
        #endif
        QTimer* pollShutdownTimer = new QTimer(guiref);
        QObject::connect(pollShutdownTimer, SIGNAL(timeout()), guiref, SLOT(detectShutdown()));
        pollShutdownTimer->start(200);

        // Connect paymentserver that after core and UI are ready, to process any alias URIs
        QObject::connect(paymentServer, SIGNAL(receivedURI(QString)), &window, SLOT(handleURI(QString)));
        QObject::connect(applicationModelPtr.data(), SIGNAL(uiReady()), paymentServer, SLOT(uiReady()));

        if (true) // AppInit2(threadGroup))
        {
            // Put this in a block, so that the Model objects are cleaned up before calling Shutdown().
            {
//                // Get locks upfront, to make sure we can completly setup our client before core sends notifications
//                ENTER_CRITICAL_SECTION(cs_main); // no RAII
//                ENTER_CRITICAL_SECTION(pwalletMain->cs_wallet); // no RAII

//                ClientModel clientModel(&optionsModel);
//                WalletModel walletModel(pwalletMain, &optionsModel);
//                window.setClientModel(&clientModel);
//                window.setWalletModel(&walletModel);

//                InitMessage("Update balance...");

//                // Manually create a blockChangedEvent to set initial values for the UI
//                BlockChangedEvent blockChangedEvent = { nBestHeight, GetNumBlocksOfPeers(), IsInitialBlockDownload(), nNodeMode == NT_FULL ?
//                                                        pindexBest ? pindexBest->GetBlockTime() : GENESIS_BLOCK_TIME :
//                                                        pindexBestHeader ? pindexBestHeader->GetBlockTime() : GENESIS_BLOCK_TIME };
//                uiInterface.NotifyBlocksChanged(blockChangedEvent);


                if (!ShutdownRequested())
                {
                    window.loadIndex(applicationModelPtr.data()->webSocketToken());

//                  // Release lock before starting event processing, otherwise lock would never be released
//                  LEAVE_CRITICAL_SECTION(pwalletMain->cs_wallet);
//                  LEAVE_CRITICAL_SECTION(cs_main);

                    qDebug() << "Start main loop";
                    app.exec();
                }

                window.hide();
//                window.setClientModel(0);
//                window.setWalletModel(0);
                guiref = 0;
            }
            LogPrintf("QT shutdown.\n\n");
#ifndef ANDROID
            // Shutdown the core and its threads, but don't exit Qt here
            std::cout << "interrupt_all\n";
            threadGroup.interrupt_all();
            std::cout << "join_all\n";
            threadGroup.join_all();
            std::cout << "Shutdown\n";
            Shutdown();
#endif
        } else
        {
#ifndef ANDROID
            threadGroup.interrupt_all();
            threadGroup.join_all();
            Shutdown();
#endif
            return 1;
        };
        paymentServiceRef = 0;
    } catch (std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(NULL);
    }
    return 0;
}


#ifdef ANDROID
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_org_alias_wallet_AliasActivity_receiveURI(JNIEnv *env, jobject obj, jstring url)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    Q_UNUSED (obj)
    if (paymentServiceRef)
        QMetaObject::invokeMethod(paymentServiceRef, "serveURI", Qt::QueuedConnection,
                                  Q_ARG(QString, urlStr));
    else {
        qDebug() << "Payment URI could not be processed because paymentServer is not ready";
    }
    env->ReleaseStringUTFChars(url, urlStr);
    return;
}

JNIEXPORT void JNICALL
Java_org_alias_wallet_AliasActivity_updateBootstrapState(JNIEnv *env, jobject obj, jint state, jint errorCode, jint progress, jint indexOfItem, jint numOfItems, jboolean indeterminate)
{
    //qDebug() << "JNI updateBootstrapState: state=" << state << " progress="<< progress << " indeterminate=" << indeterminate;
    Q_UNUSED (obj)
    if (bootstrapWizard)
        QMetaObject::invokeMethod(bootstrapWizard->page(BootstrapWizard::Page_Download), "updateBootstrapState", Qt::QueuedConnection,
                                  Q_ARG(int, state),  Q_ARG(int, errorCode), Q_ARG(int, progress), Q_ARG(int, indexOfItem), Q_ARG(int, numOfItems),  Q_ARG(bool, indeterminate));
    else {
        qDebug() << "Could not update Boostrap state because bootstrapWizard is not set";
    }
    return;
}

JNIEXPORT void JNICALL
Java_org_alias_wallet_AliasService_setCoreRunningMode(JNIEnv *env, jobject obj, jint jCoreRunningMode)
{
    qDebug() << "JNI setCoreRunningMode: jCoreRunningMode=" << jCoreRunningMode;
    Q_UNUSED (obj)
    if (jCoreRunningMode == CoreRunningMode::SLEEP)
        coreRunningMode = CoreRunningMode::REQUEST_SYNC_SLEEP;
    else
        coreRunningMode = (CoreRunningMode)jCoreRunningMode;
    if (applicationModelRef)
        QMetaObject::invokeMethod(applicationModelRef, "updateUIpaused", Qt::QueuedConnection,
                                  Q_ARG(bool, coreRunningMode != CoreRunningMode::RUNNING_NORMAL));
    return;
}

#ifdef __cplusplus
}
#endif
#endif


#endif // SPECTRE_QT_TEST
