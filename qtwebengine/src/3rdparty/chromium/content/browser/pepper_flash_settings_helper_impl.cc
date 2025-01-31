// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/pepper_flash_settings_helper_impl.h"

#include "base/files/file_path.h"
#include "content/browser/plugin_service_impl.h"
#include "content/public/browser/browser_thread.h"
#include "ipc/ipc_channel_handle.h"

namespace content {

// static
scoped_refptr<PepperFlashSettingsHelper> PepperFlashSettingsHelper::Create() {
  return new PepperFlashSettingsHelperImpl();
}

PepperFlashSettingsHelperImpl::PepperFlashSettingsHelperImpl() {
}

PepperFlashSettingsHelperImpl::~PepperFlashSettingsHelperImpl() {
}

void PepperFlashSettingsHelperImpl::OpenChannelToBroker(
    const base::FilePath& path,
    const OpenChannelCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (callback.is_null())
    return;
  if (!callback_.is_null())
    callback.Run(false, IPC::ChannelHandle());

  // Balanced in OnPpapiChannelOpened(). We need to keep this object around
  // until then.
  AddRef();

  callback_ = callback;
  PluginServiceImpl* plugin_service = PluginServiceImpl::GetInstance();
  plugin_service->OpenChannelToPpapiBroker(0, path, this);
}

void PepperFlashSettingsHelperImpl::GetPpapiChannelInfo(
    base::ProcessHandle* renderer_handle,
    int* renderer_id) {
  *renderer_handle = base::kNullProcessHandle;
  *renderer_id = 0;
}

void PepperFlashSettingsHelperImpl::OnPpapiChannelOpened(
    const IPC::ChannelHandle& channel_handle,
    base::ProcessId /* plugin_pid */,
    int /* plugin_child_id */) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(!callback_.is_null());

  if (!channel_handle.name.empty())
    callback_.Run(true, channel_handle);
  else
    callback_.Run(false, IPC::ChannelHandle());

  callback_.Reset();
  // Balance the AddRef() call in Initialize().
  Release();
}

bool PepperFlashSettingsHelperImpl::OffTheRecord() {
  return false;
}

}  // namespace content
