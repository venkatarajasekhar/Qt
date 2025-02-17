// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRYPTO_P224_SPAKE_H_
#define CRYPTO_P224_SPAKE_H_

#include <base/strings/string_piece.h>
#include <crypto/p224.h>
#include <crypto/sha2.h>

namespace crypto {

// P224EncryptedKeyExchange implements SPAKE2, a variant of Encrypted
// Key Exchange. It allows two parties that have a secret common
// password to establish a common secure key by exchanging messages
// over unsecure channel without disclosing the password.
//
// The password can be low entropy as authenticating with an attacker only
// gives the attacker a one-shot password oracle. No other information about
// the password is leaked. (However, you must be sure to limit the number of
// permitted authentication attempts otherwise they get many one-shot oracles.)
//
// The protocol requires several RTTs (actually two, but you shouldn't assume
// that.) To use the object, call GetMessage() and pass that message to the
// peer. Get a message from the peer and feed it into ProcessMessage. Then
// examine the return value of ProcessMessage:
//   kResultPending: Another round is required. Call GetMessage and repeat.
//   kResultFailed: The authentication has failed. You can get a human readable
//       error message by calling error().
//   kResultSuccess: The authentication was successful.
//
// In each exchange, each peer always sends a message.
class CRYPTO_EXPORT P224EncryptedKeyExchange {
 public:
  enum Result {
    kResultPending,
    kResultFailed,
    kResultSuccess,
  };

  // PeerType's values are named client and server due to convention. But
  // they could be called "A" and "B" as far as the protocol is concerned so
  // long as the two parties don't both get the same label.
  enum PeerType {
    kPeerTypeClient,
    kPeerTypeServer,
  };

  // peer_type: the type of the local authentication party.
  // password: secret session password. Both parties to the
  //     authentication must pass the same value. For the case of a
  //     TLS connection, see RFC 5705.
  P224EncryptedKeyExchange(PeerType peer_type,
                           const base::StringPiece& password);

  // GetMessage returns a byte string which must be passed to the other party
  // in the authentication.
  const std::string& GetMessage();

  // ProcessMessage processes a message which must have been generated by a
  // call to GetMessage() by the other party.
  Result ProcessMessage(const base::StringPiece& message);

  // In the event that ProcessMessage() returns kResultFailed, error will
  // return a human readable error message.
  const std::string& error() const;

  // The key established as result of the key exchange. Must be called
  // at then end after ProcessMessage() returns kResultSuccess.
  const std::string& GetKey();

 private:
  // The authentication state machine is very simple and each party proceeds
  // through each of these states, in order.
  enum State {
    kStateInitial,
    kStateRecvDH,
    kStateSendHash,
    kStateRecvHash,
    kStateDone,
  };

  State state_;
  const bool is_server_;
  // next_message_ contains a value for GetMessage() to return.
  std::string next_message_;
  std::string error_;

  // CalculateHash computes the verification hash for the given peer and writes
  // |kSHA256Length| bytes at |out_digest|.
  void CalculateHash(
      PeerType peer_type,
      const std::string& client_masked_dh,
      const std::string& server_masked_dh,
      const std::string& k,
      uint8* out_digest);

  // x_ is the secret Diffie-Hellman exponent (see paper referenced in .cc
  // file).
  uint8 x_[p224::kScalarBytes];
  // pw_ is SHA256(P(password), P(session))[:28] where P() prepends a uint32,
  // big-endian length prefix (see paper refereneced in .cc file).
  uint8 pw_[p224::kScalarBytes];
  // expected_authenticator_ is used to store the hash value expected from the
  // other party.
  uint8 expected_authenticator_[kSHA256Length];

  std::string key_;
};

}  // namespace crypto

#endif  // CRYPTO_P224_SPAKE_H_
