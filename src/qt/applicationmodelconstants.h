// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef APPLICATIONMODELCONSTANTS_H
#define APPLICATIONMODELCONSTANTS_H

enum ShutdownFlags {
    NORMAL = 0,
    RESET_BLOCKCHAIN  = (1U << 1),
    REWIND_BLOCKCHAIN  = (1U << 2)
};

#endif // APPLICATIONMODELCONSTANTS_H
