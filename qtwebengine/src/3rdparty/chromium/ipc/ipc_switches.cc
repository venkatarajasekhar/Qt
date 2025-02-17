// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/ipc_switches.h"

#include "base/base_switches.h"

namespace switches {

// Can't find the switch you are looking for? try looking in
// base/base_switches.cc instead.

// The value of this switch tells the child process which
// IPC channel the browser expects to use to communicate with it.
const char kProcessChannelID[]              = "channel";

}  // namespace switches

