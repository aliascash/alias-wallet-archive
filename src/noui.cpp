// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2010 Satoshi Nakamoto
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "ui_interface.h"
#include "init.h"
#include "rpcserver.h"

#include <string>

static int noui_ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style)
{
    LogPrintf("%s: %s\n", caption.c_str(), message.c_str());
    fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
    return 4;
}

static bool noui_ThreadSafeAskFee(int64_t nFeeRequired, const std::string& strCaption)
{
    return true;
}

void noui_connect()
{
    // Connect bitcoind signal handlers
    uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBox);
    uiInterface.ThreadSafeAskFee.connect(noui_ThreadSafeAskFee);
}
