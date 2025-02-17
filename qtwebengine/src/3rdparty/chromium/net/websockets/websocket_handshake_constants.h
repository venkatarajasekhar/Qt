// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A set of common constants that are needed for the WebSocket handshake.
// In general, you should prefer using these constants to literal strings,
// except in tests.
//
// These constants cannot be used in files that are compiled on iOS, because
// this file is not compiled on iOS.

#ifndef NET_WEBSOCKETS_WEBSOCKET_HANDSHAKE_CONSTANTS_H_
#define NET_WEBSOCKETS_WEBSOCKET_HANDSHAKE_CONSTANTS_H_

#include "base/basictypes.h"

// This file plases constants inside the ::net::websockets namespace to avoid
// risk of collisions with other symbols in libnet.
namespace net {
namespace websockets {

// "HTTP/1.1"
// RFC6455 only requires HTTP/1.1 "or better" but in practice an HTTP version
// other than 1.1 should not occur in a WebSocket handshake.
extern const char* const kHttpProtocolVersion;

// The Sec-WebSockey-Key challenge is 16 random bytes, base64 encoded.
extern const size_t kRawChallengeLength;

// "Sec-WebSocket-Protocol"
extern const char* const kSecWebSocketProtocol;

// "Sec-WebSocket-Extensions"
extern const char* const kSecWebSocketExtensions;

// "Sec-WebSocket-Key"
extern const char* const kSecWebSocketKey;

// "Sec-WebSocket-Accept"
extern const char* const kSecWebSocketAccept;

// "Sec-WebSocket-Version"
extern const char* const kSecWebSocketVersion;

// This implementation only supports one version of the WebSocket protocol,
// "13", as specified in RFC6455. If support for multiple versions is added in
// future, it will probably no longer be worth having a constant for this.
extern const char* const kSupportedVersion;

// "Upgrade"
extern const char* const kUpgrade;

// "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" as defined in section 4.1 of
// RFC6455.
extern const char* const kWebSocketGuid;

// Colon-prefixed lowercase headers for SPDY3.

// ":sec-websocket-protocol"
extern const char* const kSecWebSocketProtocolSpdy3;

// ":sec-websocket-extensions"
extern const char* const kSecWebSocketExtensionsSpdy3;

// Some parts of the code require lowercase versions of the header names in
// order to do case-insensitive comparisons, or because of SPDY.
// "sec-websocket-protocol"
extern const char* const kSecWebSocketProtocolLowercase;

// "sec-websocket-extensions"
extern const char* const kSecWebSocketExtensionsLowercase;

// "sec-webSocket-key"
extern const char* const kSecWebSocketKeyLowercase;

// "sec-websocket-version"
extern const char* const kSecWebSocketVersionLowercase;

// "upgrade"
extern const char* const kUpgradeLowercase;

// "websocket", as used in the "Upgrade:" header. This is always lowercase
// (except in obsolete versions of the protocol).
extern const char* const kWebSocketLowercase;

}  // namespace websockets
}  // namespace net

#endif  // NET_WEBSOCKETS_WEBSOCKET_HANDSHAKE_CONSTANTS_H_
