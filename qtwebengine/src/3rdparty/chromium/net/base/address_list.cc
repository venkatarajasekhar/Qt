// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/address_list.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "net/base/net_util.h"
#include "net/base/sys_addrinfo.h"

namespace net {

namespace {

base::Value* NetLogAddressListCallback(const AddressList* address_list,
                                       NetLog::LogLevel log_level) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  base::ListValue* list = new base::ListValue();

  for (AddressList::const_iterator it = address_list->begin();
       it != address_list->end(); ++it) {
    list->Append(new base::StringValue(it->ToString()));
  }

  dict->Set("address_list", list);
  return dict;
}

}  // namespace

AddressList::AddressList() {}

AddressList::~AddressList() {}

AddressList::AddressList(const IPEndPoint& endpoint) {
  push_back(endpoint);
}

// static
AddressList AddressList::CreateFromIPAddress(const IPAddressNumber& address,
                                             uint16 port) {
  return AddressList(IPEndPoint(address, port));
}

// static
AddressList AddressList::CreateFromIPAddressList(
    const IPAddressList& addresses,
    const std::string& canonical_name) {
  AddressList list;
  list.set_canonical_name(canonical_name);
  for (IPAddressList::const_iterator iter = addresses.begin();
       iter != addresses.end(); ++iter) {
    list.push_back(IPEndPoint(*iter, 0));
  }
  return list;
}

// static
AddressList AddressList::CreateFromAddrinfo(const struct addrinfo* head) {
  DCHECK(head);
  AddressList list;
  if (head->ai_canonname)
    list.set_canonical_name(std::string(head->ai_canonname));
  for (const struct addrinfo* ai = head; ai; ai = ai->ai_next) {
    IPEndPoint ipe;
    // NOTE: Ignoring non-INET* families.
    if (ipe.FromSockAddr(ai->ai_addr, ai->ai_addrlen))
      list.push_back(ipe);
    else
      DLOG(WARNING) << "Unknown family found in addrinfo: " << ai->ai_family;
  }
  return list;
}

// static
AddressList AddressList::CopyWithPort(const AddressList& list, uint16 port) {
  AddressList out;
  out.set_canonical_name(list.canonical_name());
  for (size_t i = 0; i < list.size(); ++i)
    out.push_back(IPEndPoint(list[i].address(), port));
  return out;
}

void AddressList::SetDefaultCanonicalName() {
  DCHECK(!empty());
  set_canonical_name(front().ToStringWithoutPort());
}

NetLog::ParametersCallback AddressList::CreateNetLogCallback() const {
  return base::Bind(&NetLogAddressListCallback, this);
}

}  // namespace net
