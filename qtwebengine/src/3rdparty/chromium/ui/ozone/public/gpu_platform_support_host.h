// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_GPU_PLATFORM_SUPPORT_HOST_H_
#define UI_OZONE_PUBLIC_GPU_PLATFORM_SUPPORT_HOST_H_

#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"
#include "ui/ozone/ozone_base_export.h"

namespace ui {

// Platform-specific object to support a GPU process host.
//
// ChromeOS on bare hardware will do display configuration and cursor
// movement from the GPU process. This provides a conduit for the
// messages needed to make this work.
//
// Under X11, we don't need any GPU messages for display configuration.
// That's why there's no real functionality here: it's purely mechanism
// to support additional messages needed by specific platforms.
class OZONE_BASE_EXPORT GpuPlatformSupportHost : public IPC::Listener {
 public:
  GpuPlatformSupportHost();
  virtual ~GpuPlatformSupportHost();

  // Called when the GPU process is spun up & channel established.
  virtual void OnChannelEstablished(int host_id, IPC::Sender* sender) = 0;

  // Called when the GPU process is destroyed.
  virtual void OnChannelDestroyed(int host_id) = 0;
};

// create a stub implementation.
OZONE_BASE_EXPORT GpuPlatformSupportHost* CreateStubGpuPlatformSupportHost();

}  // namespace ui

#endif  // UI_OZONE_PUBLIC_GPU_PLATFORM_SUPPORT_HOST_H_
