// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_ZYGOTE_COMMANDS_LINUX_H_
#define CONTENT_COMMON_ZYGOTE_COMMANDS_LINUX_H_

#include "base/posix/global_descriptors.h"
#include "ipc/ipc_descriptors.h"

namespace content {

// Contents of the initial message sent from the zygote to the browser right
// after it starts.
static const char kZygoteBootMessage[] = "ZYGOTE_BOOT";

// Contents of the initial message sent from the zygote to the browser when it
// is ready to go.
static const char kZygoteHelloMessage[] = "ZYGOTE_OK";

// Message sent by zygote children to the browser so the browser can discover
// the sending child's process ID.
static const char kZygoteChildPingMessage[] = "CHILD_PING";

// Maximum allowable length for messages sent to the zygote.
const size_t kZygoteMaxMessageLength = 8192;

// File descriptors initialized by the Zygote Host
const int kZygoteSocketPairFd =
    kPrimaryIPCChannel + base::GlobalDescriptors::kBaseDescriptor;

// These are the command codes used on the wire between the browser and the
// zygote.
enum {
  // Fork off a new renderer.
  kZygoteCommandFork = 0,

  // Reap a renderer child.
  kZygoteCommandReap = 1,

  // Check what happened to a child process.
  kZygoteCommandGetTerminationStatus = 2,

  // Read a bitmask of kSandboxLinux*
  kZygoteCommandGetSandboxStatus = 3,

  // Not a real zygote command, but a subcommand used during the zygote fork
  // protocol.  Sends the child's PID as seen from the browser process.
  kZygoteCommandForkRealPID = 4
};

}  // namespace content

#endif  // CONTENT_COMMON_ZYGOTE_COMMANDS_LINUX_H_
