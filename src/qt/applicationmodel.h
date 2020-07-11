// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include <rep_applicationmodelremote_source.h>

/** Model for Bitcoin network client. */
class ApplicationModel : public ApplicationModelRemoteSimpleSource
{
    Q_OBJECT
public:
    explicit ApplicationModel(QObject *parent = 0);
    ~ApplicationModel();

    //! Requests to shutdown the core with the given actions after shutdown
    void requestShutdownCore(unsigned int flags = NORMAL);

    // Executes logic after the shutdown of the core. Clients should never call this but requestShutdownCore() instead.
    void AfterShutdown();

private slots:
    void detectShutdown();

private:
    unsigned int shutdownFlags;
};

#endif // APPLICATIONMODEL_H
