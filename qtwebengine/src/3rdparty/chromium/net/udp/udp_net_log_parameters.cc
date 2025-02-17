// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/udp/udp_net_log_parameters.h"

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "net/base/ip_endpoint.h"

namespace net {

namespace {

base::Value* NetLogUDPDataTranferCallback(int byte_count,
                                          const char* bytes,
                                          const IPEndPoint* address,
                                          NetLog::LogLevel log_level) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetInteger("byte_count", byte_count);
  if (NetLog::IsLoggingBytes(log_level))
    dict->SetString("hex_encoded_bytes", base::HexEncode(bytes, byte_count));
  if (address)
    dict->SetString("address", address->ToString());
  return dict;
}

base::Value* NetLogUDPConnectCallback(const IPEndPoint* address,
                                      NetLog::LogLevel /* log_level */) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetString("address", address->ToString());
  return dict;
}

}  // namespace

NetLog::ParametersCallback CreateNetLogUDPDataTranferCallback(
    int byte_count,
    const char* bytes,
    const IPEndPoint* address) {
  DCHECK(bytes);
  return base::Bind(&NetLogUDPDataTranferCallback, byte_count, bytes, address);
}

NetLog::ParametersCallback CreateNetLogUDPConnectCallback(
    const IPEndPoint* address) {
  DCHECK(address);
  return base::Bind(&NetLogUDPConnectCallback, address);
}

}  // namespace net
