// Copyright (c) 2014-2016, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <cstddef>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <vector>

#include "generic-ops.h"
#include "hash.h"

namespace crypto {

  extern boost::mutex random_lock;

  class crypto_ops {
    crypto_ops();
    crypto_ops(const crypto_ops &);
    void operator=(const crypto_ops &);
    ~crypto_ops();

    static bool generate_key_derivation(const public_key &, const secret_key &, key_derivation &);
    friend bool generate_key_derivation(const public_key &, const secret_key &, key_derivation &);
    static bool derive_public_key(const key_derivation &, std::size_t, const public_key &, public_key &);
    friend bool derive_public_key(const key_derivation &, std::size_t, const public_key &, public_key &);
    static void derive_secret_key(const key_derivation &, std::size_t, const secret_key &, secret_key &);
    friend void derive_secret_key(const key_derivation &, std::size_t, const secret_key &, secret_key &);

  /* To generate an ephemeral key used to send money to:
   * * The sender generates a new key pair, which becomes the transaction key. The public transaction key is included in "extra" field.
   * * Both the sender and the receiver generate key derivation from the transaction key, the receivers' "view" key and the output index.
   * * The sender uses key derivation and the receivers' "spend" key to derive an ephemeral public key.
   * * The receiver can either derive the public key (to check that the transaction is addressed to him) or the private key (to spend the money).
   */
  inline bool generate_key_derivation(const public_key &key1, const secret_key &key2, key_derivation &derivation) {
    return crypto_ops::generate_key_derivation(key1, key2, derivation);
  }
  inline bool derive_public_key(const key_derivation &derivation, std::size_t output_index,
    const public_key &base, public_key &derived_key) {
    return crypto_ops::derive_public_key(derivation, output_index, base, derived_key);
  }
  inline void derive_secret_key(const key_derivation &derivation, std::size_t output_index,
    const secret_key &base, secret_key &derived_key) {
    crypto_ops::derive_secret_key(derivation, output_index, base, derived_key);
  }

  