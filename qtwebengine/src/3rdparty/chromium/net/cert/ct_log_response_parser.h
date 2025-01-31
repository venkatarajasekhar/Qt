// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_CT_LOG_RESPONSE_PARSER_H_
#define NET_CERT_CT_LOG_RESPONSE_PARSER_H_

#include "base/strings/string_piece.h"
#include "net/base/net_export.h"

namespace net {

namespace ct {
struct SignedTreeHead;

// Fills in |signed_tree_head| from its JSON representation in
// |json_signed_tree_head|.
// Returns true and fills in |signed_tree_head| if all fields are present and
// valid.Otherwise, returns false and does not modify |signed_tree_head|.
NET_EXPORT bool FillSignedTreeHead(
    const base::StringPiece& json_signed_tree_head,
    SignedTreeHead* signed_tree_head);

}  // namespace ct

}  // namespace net
#endif  // NET_CERT_CT_LOG_RESPONSE_PARSER_H_
