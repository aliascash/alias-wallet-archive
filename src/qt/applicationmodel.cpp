// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "applicationmodel.h"
#include "util.h"
#include "rpcserver.h"
#include "shutdown.h"

#include <QApplication>

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

ApplicationModel::ApplicationModel(QObject *parent) :
    ApplicationModelRemoteSimpleSource(parent),
    shutdownFlags(NORMAL)
{
    QTimer* pollShutdownTimer = new QTimer(this);
    QObject::connect(pollShutdownTimer, SIGNAL(timeout()), this, SLOT(detectShutdown()));
    pollShutdownTimer->start(200);
}

ApplicationModel::~ApplicationModel()
{

}

void ApplicationModel::detectShutdown()
{
    if (ShutdownRequested())
#ifdef ANDROID
        QtAndroid::androidService().callMethod<void>("stopCore", "()V");
#else
        QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
#endif
}

void ApplicationModel::requestShutdownCore(unsigned int flags)
{
    this->shutdownFlags = flags;
    if (shutdownFlags & REWIND_BLOCKCHAIN)
    {
        LogPrintf("REWIND_BLOCKCHAIN requested: issue 'rewindchain 100'b\n\n");
        json_spirit::Array params;
        params.push_back(100);
        QtAndroid::androidService().callMethod<void>("updateNotification", "(Ljava/lang/String;Ljava/lang/String;I)V",
                                                     QAndroidJniObject::fromString("Maintenance").object<jstring>(),
                                                     QAndroidJniObject::fromString("Rewind Blockchain...").object<jstring>(),
                                                     7);
        rewindchain(params, false);
    }
    StartShutdown();
}

void ApplicationModel::AfterShutdown()
{
    if (shutdownFlags & RESET_BLOCKCHAIN)
    {
        LogPrintf("RESET_BLOCKCHAIN requested: delete blk0001.dat, txleveldb\n\n");
        boost::filesystem::remove(GetDataDir() / "blk0001.dat");
        boost::filesystem::remove_all(GetDataDir() / "txleveldb");
    }
}
