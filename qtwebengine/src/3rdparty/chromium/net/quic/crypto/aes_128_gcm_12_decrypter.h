// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CRYPTO_AES_128_GCM_12_DECRYPTER_H_
#define NET_QUIC_CRYPTO_AES_128_GCM_12_DECRYPTER_H_

#include "net/quic/crypto/aead_base_decrypter.h"

namespace net {

// An Aes128Gcm12Decrypter is a QuicDecrypter that implements the
// AEAD_AES_128_GCM_12 algorithm specified in RFC 5282. Create an instance by
// calling QuicDecrypter::Create(kAESG).
//
// It uses an authentication tag of 12 bytes (96 bits). The fixed prefix
// of the nonce is four bytes.
class NET_EXPORT_PRIVATE Aes128Gcm12Decrypter : public AeadBaseDecrypter {
 public:
  enum {
    // Authentication tags are truncated to 96 bits.
    kAuthTagSize = 12,
  };

  Aes128Gcm12Decrypter();
  virtual ~Aes128Gcm12Decrypter();

#if !defined(USE_OPENSSL)
 protected:
  // AeadBaseDecrypter methods:
  virtual void FillAeadParams(base::StringPiece nonce,
                              base::StringPiece associated_data,
                              size_t auth_tag_size,
                              AeadParams* aead_params) const OVERRIDE;
#endif

 private:
  DISALLOW_COPY_AND_ASSIGN(Aes128Gcm12Decrypter);
};

}  // namespace net

#endif  // NET_QUIC_CRYPTO_AES_128_GCM_12_DECRYPTER_H_
