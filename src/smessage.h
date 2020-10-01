// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef SEC_MESSAGE_H
#define SEC_MESSAGE_H

#include "allocators.h"
#include <vector>


// Secure Message Crypter
class SecMsgCrypter
{
private:
    uint8_t chKey[32];
    uint8_t chIV[16];
    bool fKeySet;
public:

    SecMsgCrypter()
    {
        // Try to keep the key data out of swap (and be a bit over-careful to keep the IV that we don't even use out of swap)
        // Note that this does nothing about suspend-to-disk (which will put all our key data on disk)
        // Note as well that at no point in this program is any attempt made to prevent stealing of keys by reading the memory of the running process.
        LockedPageManager::instance.LockRange(&chKey[0], sizeof chKey);
        LockedPageManager::instance.LockRange(&chIV[0], sizeof chIV);
        fKeySet = false;
    }

    ~SecMsgCrypter()
    {
        // clean key
        memset(&chKey, 0, sizeof chKey);
        memset(&chIV, 0, sizeof chIV);
        fKeySet = false;

        LockedPageManager::instance.UnlockRange(&chKey[0], sizeof chKey);
        LockedPageManager::instance.UnlockRange(&chIV[0], sizeof chIV);
    }

    bool SetKey(const std::vector<uint8_t>& vchNewKey, uint8_t* chNewIV);
    bool SetKey(const uint8_t* chNewKey, uint8_t* chNewIV);
    bool Encrypt(uint8_t* chPlaintext,  uint32_t nPlain,  std::vector<uint8_t> &vchCiphertext);
    bool Decrypt(uint8_t* chCiphertext, uint32_t nCipher, std::vector<uint8_t>& vchPlaintext);
};

#endif // SEC_MESSAGE_H

