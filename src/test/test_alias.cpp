// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
//
// SPDX-License-Identifier: MIT

#define BOOST_TEST_MODULE Spectre Test Suite
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "state.h"
#include "db.h"
#include "main.h"
#include "wallet.h"


CWallet *pwalletMain;
CClientUIInterface uiInterface;

extern bool fPrintToConsole;
extern void noui_connect();

boost::filesystem::path pathTemp;

struct TestingSetup {
    TestingSetup() {
        //fPrintToDebugLog = false; // don't want to write to debug.log file

        //pathTemp = GetTempPath() / strprintf("test_spectre_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
        pathTemp = GetTempPath() / "test_spectre";
        //printf("pathTemp %s\n", pathTemp.string().c_str());
        boost::filesystem::create_directories(pathTemp);
        mapArgs["-datadir"] = pathTemp.string();

        fDebug = true;
        fDebugChain = true;
        fDebugRingSig = true;
        fDebugPoS = true;

        noui_connect();
        bitdb.MakeMock();

        LoadBlockIndex(true);
        pwalletMain = new CWallet("walletUT.dat");
        int oltWalletVersion = 0;
        pwalletMain->LoadWallet(oltWalletVersion, [] (const uint32_t& nBlock) -> void {
            uiInterface.InitMessage(strprintf(_("Loading wallet items... (%d)"), nBlock));
        });

        RegisterWallet(pwalletMain);
    }
    ~TestingSetup()
    {
        delete pwalletMain;
        pwalletMain = NULL;
        bitdb.Flush(true);
    }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);

void Shutdown(void* parg)
{
    exit(0);
}

void StartShutdown()
{
    exit(0);
}
