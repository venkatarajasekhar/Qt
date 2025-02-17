// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "native_client/src/public/chrome_main.h"
#include "native_client/src/public/irt_core.h"
#include "ppapi/nacl_irt/irt_ppapi.h"
#include "ppapi/nacl_irt/plugin_startup.h"

void nacl_irt_start(uint32_t* info) {
  nacl_irt_init(info);

  // Though it isn't referenced here, we must instantiate an AtExitManager.
  base::AtExitManager exit_manager;

  // In SFI mode, the FDs of IPC channels are NACL_CHROME_DESC_BASE and its
  // successor, which is set in nacl_listener.cc.
  ppapi::SetIPCFileDescriptors(
      NACL_CHROME_DESC_BASE,
      NACL_CHROME_DESC_BASE + 1,
      -1);  // Currently manifest service is disabled on NaCl in SFI mode.
  ppapi::StartUpPlugin();

  nacl_irt_enter_user_code(info, chrome_irt_query);
}
