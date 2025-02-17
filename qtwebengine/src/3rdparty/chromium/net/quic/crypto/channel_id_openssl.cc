// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/crypto/channel_id.h"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include "crypto/openssl_util.h"

using base::StringPiece;

namespace net {

// static
bool ChannelIDVerifier::Verify(StringPiece key,
                               StringPiece signed_data,
                               StringPiece signature) {
  return VerifyRaw(key, signed_data, signature, true);
}

// static
bool ChannelIDVerifier::VerifyRaw(StringPiece key,
                                  StringPiece signed_data,
                                  StringPiece signature,
                                  bool is_channel_id_signature) {
  if (key.size() != 32 * 2 ||
      signature.size() != 32 * 2) {
    return false;
  }

  crypto::ScopedOpenSSL<EC_GROUP, EC_GROUP_free> p256(
      EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
  if (p256.get() == NULL) {
    return false;
  }

  crypto::ScopedOpenSSL<BIGNUM, BN_free> x(BN_new()), y(BN_new()),
                                         r(BN_new()), s(BN_new());

  ECDSA_SIG sig;
  sig.r = r.get();
  sig.s = s.get();

  const uint8* key_bytes = reinterpret_cast<const uint8*>(key.data());
  const uint8* signature_bytes =
      reinterpret_cast<const uint8*>(signature.data());

  if (BN_bin2bn(key_bytes       +  0, 32, x.get()) == NULL ||
      BN_bin2bn(key_bytes       + 32, 32, y.get()) == NULL ||
      BN_bin2bn(signature_bytes +  0, 32, sig.r) == NULL ||
      BN_bin2bn(signature_bytes + 32, 32, sig.s) == NULL) {
    return false;
  }

  crypto::ScopedOpenSSL<EC_POINT, EC_POINT_free> point(
      EC_POINT_new(p256.get()));
  if (point.get() == NULL ||
      !EC_POINT_set_affine_coordinates_GFp(p256.get(), point.get(), x.get(),
                                           y.get(), NULL)) {
    return false;
  }

  crypto::ScopedOpenSSL<EC_KEY, EC_KEY_free> ecdsa_key(EC_KEY_new());
  if (ecdsa_key.get() == NULL ||
      !EC_KEY_set_group(ecdsa_key.get(), p256.get()) ||
      !EC_KEY_set_public_key(ecdsa_key.get(), point.get())) {
    return false;
  }

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  if (is_channel_id_signature) {
    SHA256_Update(&sha256, kContextStr, strlen(kContextStr) + 1);
    SHA256_Update(&sha256, kClientToServerStr, strlen(kClientToServerStr) + 1);
  }
  SHA256_Update(&sha256, signed_data.data(), signed_data.size());

  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256_Final(digest, &sha256);

  return ECDSA_do_verify(digest, sizeof(digest), &sig, ecdsa_key.get()) == 1;
}

}  // namespace net
