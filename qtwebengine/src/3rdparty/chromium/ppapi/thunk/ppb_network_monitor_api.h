// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_NETWORK_MONITOR_API_H_
#define PPAPI_THUNK_PPB_NETWORK_MONITOR_API_H_

#include "ppapi/c/ppb_network_monitor.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_NetworkMonitor_API {
 public:
  virtual ~PPB_NetworkMonitor_API() {}

  virtual int32_t UpdateNetworkList(
      PP_Resource* network_list,
      scoped_refptr<TrackedCallback> callback) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_NETWORK_MONITOR_API_H_
