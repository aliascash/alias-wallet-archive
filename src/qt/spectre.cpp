// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
//
// SPDX-License-Identifier: MIT

#include "spectregui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "paymentserver.h"
#include "winshutdownmonitor.h"
#include "setupwalletwizard.h"

#include "websocketclientwrapper.h"
#include "websockettransport.h"

#include "init.h"
#include "interface.h"

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



#ifndef WIN32
#include <signal.h>
#endif

namespace fs = boost::filesystem;

// Need a global reference for the notifications to find the GUI
static SpectreGUI *guiref;
static QSplashScreen *splashref;

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
    if(splashref)
    {
        splashref->showMessage(QString::fromStdString("v"+FormatClientVersion()) + "\n" + QString::fromStdString(message) + "\n", Qt::AlignBottom|Qt::AlignHCenter, QColor(138,140,142));
        QApplication::instance()->processEvents();
    }
}

/*
   Translate string to current locale using Qt.
 */
static std::string Translate(const char* psz)
{
    return QCoreApplication::translate("alias-core", psz).toStdString();
}

/* Handle runaway exceptions. Shows a message box with the problem and quits the program.
 */
static void handleRunawayException(std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    QMessageBox::critical(0, "Runaway exception", SpectreGUI::tr("A fatal error occurred. Alias can no longer continue safely and will quit.") + QString("\n\n") + QString::fromStdString(strMiscWarning));
    exit(1);
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

#if QT_VERSION < 0x050000
    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    // Command-line options take precedence:
    ParseParameters(argc, argv);

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
    uiInterface.ThreadSafeAskFee.connect(ThreadSafeAskFee);
    uiInterface.ThreadSafeHandleURI.connect(ThreadSafeHandleURI);
    uiInterface.InitMessage.connect(InitMessage);
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

    // Start SetupWalletWizard if no wallet.dat exists
#ifndef ANDROID // temporarly remove setupwizard until fixed for android
    if (!mapArgs.count("-bip44key") && !mapArgs.count("-wallet") && !mapArgs.count("-salvagewallet") && !fs::exists(GetDataDir() / "wallet.dat"))
    {
        SetupWalletWizard wizard;
        wizard.show();
        if (!wizard.exec())
            return 0;
        if (wizard.hasVisitedPage(SetupWalletWizard::Page_RecoverFromMnemonic))
            SoftSetBoolArg("-rescan", true);
    }
#endif

    QSplashScreen splash(GUIUtil::createPixmap(600, 686, QColor(40, 40, 41), QString(":/assets/svg/Alias-Stacked-Reverse.svg"), QRect(62, 87, 476, 476)));
    if (GetBoolArg("-splash", true) && !GetBoolArg("-min"))
    {
        splash.setEnabled(false);
        splash.show();
        splashref = &splash;
    }

    app.processEvents();

    app.setQuitOnLastWindowClosed(false);

    //---- Create webSocket server for JavaScript client
    QWebSocketServer server(
        QStringLiteral("Alias Websocket Server"),
        QWebSocketServer::NonSecureMode
    );
    if (!server.listen(QHostAddress::LocalHost, fTestNet ? WEBSOCKETPORT_TESTNET : WEBSOCKETPORT)) {
        qFatal("QWebSocketServer failed to listen on port 52471");
        return 1;
    }
    qDebug() << "QWebSocketServer started: " << server.serverAddress() << ":" << server.serverPort();

    // wrap WebSocket clients in QWebChannelAbstractTransport objects
    QString webSocketToken = QUuid::createUuid().toString().remove(QChar('{')).remove(QChar('}'));
    // qDebug() << "QWebSocketServer access token: " << webSocketToken;
    WebSocketClientWrapper clientWrapper(&server, webSocketToken);

    // setup the channel
    QWebChannel webChannel;
    QObject::connect(&clientWrapper, &WebSocketClientWrapper::clientConnected,
                     &webChannel, &QWebChannel::connectTo);

    try
    {
        // Regenerate startup link, to fix links to old versions
        if (GUIUtil::GetStartOnSystemStartup())
            GUIUtil::SetStartOnSystemStartup(true);

        boost::thread_group threadGroup;

        SpectreGUI window(&webChannel);
        window.setSplashScreen(&splash);
        guiref = &window;

        // Periodically check if shutdown was requested to properly quit the Qt application
        #if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("Alias Core did't yet exit safely..."), (HWND)window.winId());
        #endif
        QTimer* pollShutdownTimer = new QTimer(guiref);
        QObject::connect(pollShutdownTimer, SIGNAL(timeout()), guiref, SLOT(detectShutdown()));
        pollShutdownTimer->start(200);

        if (AppInit2(threadGroup))
        {
            // Put this in a block, so that the Model objects are cleaned up before calling Shutdown().
            {
                // Get locks upfront, to make sure we can completly setup our client before core sends notifications
                ENTER_CRITICAL_SECTION(cs_main); // no RAII
                ENTER_CRITICAL_SECTION(pwalletMain->cs_wallet); // no RAII

                paymentServer->setOptionsModel(&optionsModel);

                ClientModel clientModel(&optionsModel);
                WalletModel walletModel(pwalletMain, &optionsModel);
                window.setClientModel(&clientModel);
                window.setWalletModel(&walletModel);

                InitMessage("Update balance...");

                // Manually create a blockChangedEvent to set initial values for the UI
                BlockChangedEvent blockChangedEvent = { nBestHeight, GetNumBlocksOfPeers(), IsInitialBlockDownload(), nNodeMode == NT_FULL ?
                                                        pindexBest ? pindexBest->GetBlockTime() : GENESIS_BLOCK_TIME :
                                                        pindexBestHeader ? pindexBestHeader->GetBlockTime() : GENESIS_BLOCK_TIME };
                uiInterface.NotifyBlocksChanged(blockChangedEvent);

                // Check if wallet unlock is needed to determine current balance
                if (pwalletMain->IsLocked())
                {
                    WalletModel::UnlockContext unlockContext = walletModel.requestUnlock(WalletModel::UnlockMode::login);
                    if (!unlockContext.isValid())
                    {
                        InitMessage("Shutdown...");
                        StartShutdown();
                    }
                }

                if (!ShutdownRequested())
                {
                    InitMessage("...Start UI...");
                    window.loadIndex(webSocketToken);

                    // Now that initialization/startup is done, process any command-line
                    // spectre: URIs
                    QObject::connect(paymentServer, SIGNAL(receivedURI(QString)), &window, SLOT(handleURI(QString)));
                    QTimer::singleShot(100, paymentServer, SLOT(uiReady()));
                }

               // Release lock before starting event processing, otherwise lock would never be released
               LEAVE_CRITICAL_SECTION(pwalletMain->cs_wallet);
               LEAVE_CRITICAL_SECTION(cs_main);

                if (!ShutdownRequested())
                    app.exec();
                else
                    QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);

                window.hide();
                window.setClientModel(0);
                window.setWalletModel(0);
                guiref = 0;
            }
            // Shutdown the core and its threads, but don't exit Qt here
            LogPrintf("Alias shutdown.\n\n");
            std::cout << "interrupt_all\n";
            threadGroup.interrupt_all();
            std::cout << "join_all\n";
            threadGroup.join_all();
            std::cout << "Shutdown\n";
            Shutdown();
        } else
        {
            threadGroup.interrupt_all();
            threadGroup.join_all();
            Shutdown();
            return 1;
        };
    } catch (std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(NULL);
    }
    return 0;
}
#endif // SPECTRE_QT_TEST
