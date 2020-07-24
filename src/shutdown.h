// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
// SPDX-FileCopyrightText: © 2009 Satoshi Nakamoto
//
// SPDX-License-Identifier: MIT

#ifndef BITCOIN_SHUTDOWN_H
#define BITCOIN_SHUTDOWN_H

void StartShutdown();
void AbortShutdown();
bool ShutdownRequested();

#endif
