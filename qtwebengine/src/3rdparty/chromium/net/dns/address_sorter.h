// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_ADDRESS_SORTER_H_
#define NET_DNS_ADDRESS_SORTER_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/net_export.h"

namespace net {

class AddressList;

// Sorts AddressList according to RFC3484, by likelihood of successful
// connection. Depending on the platform, the sort could be performed
// asynchronously by the OS, or synchronously by local implementation.
// AddressSorter does not necessarily preserve port numbers on the sorted list.
class NET_EXPORT AddressSorter {
 public:
  typedef base::Callback<void(bool success,
                              const AddressList& list)> CallbackType;

  virtual ~AddressSorter() {}

  // Sorts |list|, which must include at least one IPv6 address.
  // Calls |callback| upon completion. Could complete synchronously. Could
  // complete after this AddressSorter is destroyed.
  virtual void Sort(const AddressList& list,
                    const CallbackType& callback) const = 0;

  // Creates platform-dependent AddressSorter.
  static scoped_ptr<AddressSorter> CreateAddressSorter();

 protected:
  AddressSorter() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AddressSorter);
};

}  // namespace net

#endif  // NET_DNS_ADDRESS_SORTER_H_
