// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "applicationmodel.h"
#include "util.h"
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
