// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_crypto_stream.h"

#include <string>

#include "base/strings/string_piece.h"
#include "net/quic/crypto/crypto_handshake.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_session.h"
#include "net/quic/quic_utils.h"

using std::string;
using base::StringPiece;

namespace net {

QuicCryptoStream::QuicCryptoStream(QuicSession* session)
    : ReliableQuicStream(kCryptoStreamId, session),
      encryption_established_(false),
      handshake_confirmed_(false) {
  crypto_framer_.set_visitor(this);
  DisableFlowControl();
}

void QuicCryptoStream::OnError(CryptoFramer* framer) {
  DLOG(WARNING) << "Error processing crypto data: "
                << QuicUtils::ErrorToString(framer->error());
}

void QuicCryptoStream::OnHandshakeMessage(
    const CryptoHandshakeMessage& message) {
  session()->OnCryptoHandshakeMessageReceived(message);
}

uint32 QuicCryptoStream::ProcessRawData(const char* data,
                                        uint32 data_len) {
  // Do not process handshake messages after the handshake is confirmed.
  if (handshake_confirmed()) {
    CloseConnection(QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE);
    return 0;
  }
  if (!crypto_framer_.ProcessInput(StringPiece(data, data_len))) {
    CloseConnection(crypto_framer_.error());
    return 0;
  }
  return data_len;
}

QuicPriority QuicCryptoStream::EffectivePriority() const {
  return QuicUtils::HighestPriority();
}

void QuicCryptoStream::SendHandshakeMessage(
    const CryptoHandshakeMessage& message) {
  session()->OnCryptoHandshakeMessageSent(message);
  const QuicData& data = message.GetSerialized();
  // TODO(wtc): check the return value.
  WriteOrBufferData(string(data.data(), data.length()), false, NULL);
}

const QuicCryptoNegotiatedParameters&
QuicCryptoStream::crypto_negotiated_params() const {
  return crypto_negotiated_params_;
}

}  // namespace net
