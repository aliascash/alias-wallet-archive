// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

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

#include "rep_applicationmodelremote_source.h"

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
static ApplicationModelRemoteSimpleSource *applicationModelRef;
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
    LogPrintf("%s\n", message);
    if (applicationModelRef)
    {
        applicationModelRef->setCoreMessage(QString::fromStdString(message));
#ifdef ANDROID
        QtAndroid::androidService().callMethod<void>("updateNotification", "(Ljava/lang/String;Ljava/lang/String;)V",
                                                     QAndroidJniObject::fromString(QObject::tr("Initializing")).object<jstring>(),
                                                     QAndroidJniObject::fromString(QString::fromStdString(message)).object<jstring>());
#endif
    }
    if(splashref)
        splashref->showMessage(QString::fromStdString("v"+FormatClientVersion()) + "\n" + QString::fromStdString(message), Qt::AlignVCenter|Qt::AlignHCenter, QColor(235,149,50));
    if (splashref || applicationModelRef)
        QApplication::instance()->processEvents();
}

static void InitQMessage(const QString &message)
{
    if(splashref)
    {
        splashref->showMessage(QString::fromStdString("v"+FormatClientVersion()) + "\n" + message, Qt::AlignVCenter|Qt::AlignHCenter, QColor(235,149,50));
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
    QMessageBox::critical(0, "Runaway exception", SpectreGUI::tr("A fatal error occurred. Spectrecoin can no longer continue safely and will quit.") + QString("\n\n") + QString::fromStdString(strMiscWarning));
    exit(1);
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
    QString nativeLibraryDir = QAndroidJniObject::getStaticObjectField<jstring>("org/spectrecoin/wallet/SpectrecoinService","nativeLibraryDir").toString();
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

        bool rescan = false;
        bool init = QtAndroid::androidService().getField<jboolean>("init");
        while(!init)
        {   // Wait for intent extras set
            QApplication::instance()->processEvents(QEventLoop::AllEvents, 100);
            init = QtAndroid::androidService().getField<jboolean>("init");
        }
        rescan = QtAndroid::androidService().getField<jboolean>("rescan");
        if (rescan) SoftSetBoolArg("-rescan", true);
        QString bip44key = QtAndroid::androidService().getObjectField<jstring>("bip44key").toString();
        if (!bip44key.isEmpty()) SoftSetArg("-bip44key", bip44key.toStdString());

        //---- Setup remote objects host
        QRemoteObjectHost srcNode(QUrl(QStringLiteral("local:spectrecoin")));
        ApplicationModelRemoteSimpleSource applicationModel;
        srcNode.enableRemoting(&applicationModel); // enable remoting
        applicationModelRef = &applicationModel;

        //---- Create core webSocket server for JavaScript client
        QWebSocketServer server(QStringLiteral("Spectrecoin Core Websocket Server"), QWebSocketServer::NonSecureMode);
        if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET : WEBSOCKETPORT))
            throw std::runtime_error(strprintf("Spectrecoin Core QWebSocketServer failed to listen on port %d", fTestNet ? WEBSOCKETPORT_TESTNET : WEBSOCKETPORT));
        qDebug() << "Core QWebSocketServer started: " << server.serverAddress() << ":" << server.serverPort();
        WebSocketClientWrapper clientWrapper(&server);  // wrap WebSocket clients in QWebChannelAbstractTransport objects
        QWebChannel webChannel;                         // setup the channel
        QObject::connect(&clientWrapper, &WebSocketClientWrapper::clientConnected, &webChannel, &QWebChannel::connectTo);

        // Initialize Core
        fRet = AppInit2(threadGroup);

        if (fRet)
        {
            // reset androidservice startup flags
            QtAndroid::androidService().setField<jboolean>("rescan", false);
            QtAndroid::androidService().setField<jstring>("bip44key", QAndroidJniObject::fromString("").object<jstring>());

            // Get locks upfront, to make sure we can completly setup our client before core sends notifications
            ENTER_CRITICAL_SECTION(cs_main); // no RAII
            ENTER_CRITICAL_SECTION(pwalletMain->cs_wallet); // no RAII

            // create models
            OptionsModel optionsModel;
            WalletModel walletModel(pwalletMain, &optionsModel);
            ClientModel clientModel(&optionsModel, &walletModel);
            SpectreBridge bridge(&webChannel);
            bridge.setClientModel(&clientModel);
            bridge.setWalletModel(&walletModel);
            bridge.setApplicationModel(&applicationModel);

            InitMessage("Update balance...");

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

            LogPrintf("SpectreCoin shutdown.\n\n");
            srcNode.disableRemoting(&applicationModel);
            threadGroup.interrupt_all();
            threadGroup.join_all();
        }
        else
        {
            LogPrintf("Init not successfull: SpectreCoin shutdown.\n\n");
            threadGroup.interrupt_all();
            // threadGroup.join_all(); was left out intentionally here, because we didn't re-test all of
            // the startup-failure cases to make sure they don't result in a hang due to some
            // thread-blocking-waiting-for-another-thread-during-startup case
        }
        applicationModelRef = 0;
        Shutdown();
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

    // Command-line options take precedence:
    ParseParameters(argc, argv);

    if (GetBoolArg("-service", false))
    {
        bool fRet = AndroidAppInit(argc, argv);
        return (fRet ? 0 : 1);
    }

    fHaveGUI = true;

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
        QMessageBox::critical(0, "Spectrecoin", QString("Error: Invalid combination of -testnet and -regtest."));
        return 1;
    }

    QApplication app(argc, argv);
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

    // ... then spectrecoin.conf:
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        // This message can not be translated, as translation is not initialized yet
        // (which not yet possible because lang=XX can be overridden in bitcoin.conf in the data directory)
        QMessageBox::critical(0, "Spectrecoin",
                              QString("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(mapArgs["-datadir"])));
        return 1;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);

    // Application identification (must be set before OptionsModel is initialized,
    // as it is used to locate QSettings)
    app.setOrganizationName("The Spectrecoin Project");
    app.setOrganizationDomain("spectreproject.io");
    if(GetBoolArg("-testnet")) // Separate UI settings for testnet
        app.setApplicationName("Spectrecoin-testnet");
    else
        app.setApplicationName("Spectrecoin");

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
        {
            SoftSetArg("-bip44key", static_cast<RecoverFromMnemonicPage*>(wizard.page(SetupWalletWizard::Page_RecoverFromMnemonic))->sKey);
            SoftSetBoolArg("-rescan", true);
        }
        else if (wizard.hasVisitedPage(SetupWalletWizard::Page_NewMnemonic_Verification))
            SoftSetArg("-bip44key", static_cast<NewMnemonicSettingsPage*>(wizard.page(SetupWalletWizard::Page_NewMnemonic_Settings))->sKey);
    }

#ifdef ANDROID
    // change android keyboard mode from adjustPan to adjustResize (note: setting adjustResize in AndroidManifest.xml and switching to adjustPan before showing SetupWalletWizard did not work)
    QtAndroid::androidActivity().callMethod<void>("setSoftInputModeAdjustResize", "()V");
    // Start core service (if service is allready running, this has not effect.)
    std::string bip44key = GetArg("-bip44key", "");
    QtAndroid::androidActivity().callMethod<void>("startCore", "(ZLjava/lang/String;)V", GetBoolArg("-rescan"), QAndroidJniObject::fromString(QString::fromStdString(bip44key)).object<jstring>());
#endif

    QSplashScreen splash(QPixmap(":/images/splash"), 0);
    if (GetBoolArg("-splash", true) && !GetBoolArg("-min"))
    {
        splash.setEnabled(false);
        splash.show();
        splashref = &splash;
    }

    app.processEvents();

    app.setQuitOnLastWindowClosed(false);

    //---- Create webSocket server for JavaScript client
    QWebSocketServer server(QStringLiteral("Spectrecoin Client Websocket Server"), QWebSocketServer::NonSecureMode);
    if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET + 1 : WEBSOCKETPORT + 1)) {
        if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET + 1: WEBSOCKETPORT + 1)) {
            qFatal("Spectrecoin Client QWebSocketServer failed to listen on port %d", fTestNet ? WEBSOCKETPORT_TESTNET + 1: WEBSOCKETPORT + 1);
            return 1;
        }
    }
    qDebug() << "Client QWebSocketServer started: " << server.serverAddress() << ":" << server.serverPort();
    WebSocketClientWrapper clientWrapper(&server); // wrap WebSocket clients in QWebChannelAbstractTransport objects
    QWebChannel webChannel;                        // setup the channel
    QObject::connect(&clientWrapper, &WebSocketClientWrapper::clientConnected, &webChannel, &QWebChannel::connectTo);

    try
    {
        // Regenerate startup link, to fix links to old versions
        if (GUIUtil::GetStartOnSystemStartup())
            GUIUtil::SetStartOnSystemStartup(true);

        boost::thread_group threadGroup;

        InitMessage("Connect service...");

        // Accuire remote objects replicas
        QRemoteObjectNode repNode;
        QSharedPointer<ClientModelRemoteReplica> clientModelPtr; // holds reference to clientmodel replica
        QSharedPointer<ApplicationModelRemoteReplica> applicationModelPtr; // holds reference to applicationmodel replica
        QSharedPointer<WalletModelRemoteReplica> walletModelPtr; // holds reference to walletmodel replica
        QSharedPointer<AddressModelRemoteReplica> addressModelPtr; // holds reference to walletmodel replica
        repNode.connectToNode(QUrl(QStringLiteral("local:spectrecoin"))); // connect with remote host node

        applicationModelPtr.reset(repNode.acquire<ApplicationModelRemoteReplica>()); // acquire replica of source from host node
        clientModelPtr.reset(repNode.acquire<ClientModelRemoteReplica>()); // acquire replica of source from host node
        walletModelPtr.reset(repNode.acquire<WalletModelRemoteReplica>()); // acquire replica of source from host node
        addressModelPtr.reset(repNode.acquire<AddressModelRemoteReplica>()); // acquire replica of source from host node

        QObject::connect(applicationModelPtr.data(), &ApplicationModelRemoteReplica::coreMessageChanged, InitQMessage);
        QObject::connect(applicationModelPtr.data(), &ApplicationModelRemoteReplica::stateChanged, RemoteModelStateChanged);

        if (!applicationModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : ApplicationModelRemoteReplica was not initialized!");
        if (!clientModelPtr->waitForSource(-1))
            throw std::runtime_error("SpectreGUI() : ClientModelRemoteReplica was not initialized!");
        if (!walletModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : WalletModelRemoteReplica was not initialized!");
        if (!addressModelPtr->waitForSource())
            throw std::runtime_error("SpectreGUI() : AddressModelRemoteReplica was not initialized!");

        SpectreGUI window(applicationModelPtr, clientModelPtr, walletModelPtr, addressModelPtr);
        window.setSplashScreen(&splash);
        guiref = &window;

        SpectreClientBridge clientBridge(&window, &webChannel);

        // Periodically check if shutdown was requested to properly quit the Qt application
        #if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("Spectrecoin Core did't yet exit safely..."), (HWND)window.winId());
        #endif
        QTimer* pollShutdownTimer = new QTimer(guiref);
        QObject::connect(pollShutdownTimer, SIGNAL(timeout()), guiref, SLOT(detectShutdown()));
        pollShutdownTimer->start(200);

        // Connect paymentserver that after core and UI are ready, to process any spectrecoin URIs
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

                // Check if wallet unlock is needed to determine current balance
                if (walletModelPtr->encryptionInfo().status() == EncryptionStatus::Locked && evaluate(walletModelPtr->countLockedAnonOutputs()) > 0)
                {
                    SpectreGUI::UnlockContext unlockContext = window.requestUnlock(SpectreGUI::UnlockMode::rescan);
                    if (!unlockContext.isValid())
                    {
                        InitMessage("Shutdown...");
                        StartShutdown();
                    }
                }

                if (!ShutdownRequested())
                {
                    window.loadIndex();
                }
 
//               // Release lock before starting event processing, otherwise lock would never be released
//               LEAVE_CRITICAL_SECTION(pwalletMain->cs_wallet);
//               LEAVE_CRITICAL_SECTION(cs_main);

                if (!ShutdownRequested())
                    app.exec();
                else
                    QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);

                window.hide();
//                window.setClientModel(0);
//                window.setWalletModel(0);
                guiref = 0;
            }
            // Shutdown the core and its threads, but don't exit Qt here
            LogPrintf("Spectrecoin QT shutdown.\n\n");
            std::cout << "interrupt_all\n";
            threadGroup.interrupt_all();
            std::cout << "join_all\n";
            threadGroup.join_all();
            std::cout << "Shutdown\n";
//            Shutdown();
        } else
        {
            threadGroup.interrupt_all();
            threadGroup.join_all();
//            Shutdown();
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
Java_org_spectrecoin_wallet_SpectrecoinActivity_receiveURI(JNIEnv *env, jobject obj, jstring url)
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
Java_org_spectrecoin_wallet_SpectrecoinActivity_updateBootstrapState(JNIEnv *env, jobject obj, jint state, jint progress, jboolean indeterminate)
{
    //qDebug() << "JNI updateBootstrapState: state=" << state << " progress="<< progress << " indeterminate=" << indeterminate;
    Q_UNUSED (obj)
    if (bootstrapWizard)
        QMetaObject::invokeMethod(bootstrapWizard->page(BootstrapWizard::Page_Download), "updateBootstrapState", Qt::QueuedConnection,
                                  Q_ARG(int, state),  Q_ARG(int, progress),  Q_ARG(bool, indeterminate));
    else {
        qDebug() << "Could not update Boostrap state because bootstrapWizard is not set";
    }
    return;
}

#ifdef __cplusplus
}
#endif
#endif


#endif // SPECTRE_QT_TEST
