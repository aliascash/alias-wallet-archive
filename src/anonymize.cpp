// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2013 StealthCoin/StealthSend Developers
// SPDX-FileCopyrightText: © 2013 BritCoin Developers
// SPDX-FileCopyrightText: © 2009 Satoshi Nakamoto
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "anonymize.h"
#include "util.h"

#include <boost/filesystem.hpp>
#include <string>
#include <cstring>

extern const char tor_git_revision[];
const char tor_git_revision[] = "";

char const* anonymize_tor_data_directory(
) {
    static std::string const retrieved = (
        GetDefaultDataDir(
        ) / "tor"
    ).string(
    );
    return retrieved.c_str(
    );
}

char const* anonymize_service_directory(
) {
    static std::string const retrieved = (
        GetDefaultDataDir(
        ) / "onion"
    ).string(
    );
    return retrieved.c_str(
    );
}
