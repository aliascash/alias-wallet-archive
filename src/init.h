// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
// SPDX-FileCopyrightText: © 2009 Satoshi Nakamoto
//
// SPDX-License-Identifier: MIT

#ifndef BITCOIN_INIT_H
#define BITCOIN_INIT_H

#include "wallet.h"
#include "shutdown.h"

extern CWallet* pwalletMain;
void Shutdown();
int BlockSignals();
bool AppInit2(boost::thread_group& threadGroup);
std::string HelpMessage();

#endif
