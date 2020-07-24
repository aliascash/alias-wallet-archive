// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2010 Satoshi Nakamoto
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef _BITCOINRPC_CLIENT_H_
#define _BITCOINRPC_CLIENT_H_ 1

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

int CommandLineRPC(int argc, char *argv[]);

json_spirit::Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams);

#endif
