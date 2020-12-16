// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include <rep_applicationmodelremote_source.h>

/** Model for Core Service. */
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

public slots:
    void updateCoreSleeping(bool sleeping);
    void updateUIpaused(bool uiPaused);

private slots:
    void detectShutdown();

private:
    unsigned int shutdownFlags;
};

#endif // APPLICATIONMODEL_H
