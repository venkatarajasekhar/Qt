// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/ipc_channel.h"

#include <limits>

#include "base/atomic_sequence_num.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"

#if !defined(OS_NACL)
namespace {

// Global atomic used to guarantee channel IDs are unique.
base::StaticAtomicSequenceNumber g_last_id;

}  // namespace
#endif

namespace IPC {

// static
std::string Channel::GenerateUniqueRandomChannelID() {
  // Note: the string must start with the current process id, this is how
  // some child processes determine the pid of the parent.
  //
  // This is composed of a unique incremental identifier, the process ID of
  // the creator, an identifier for the child instance, and a strong random
  // component. The strong random component prevents other processes from
  // hijacking or squatting on predictable channel names.

  int process_id = base::GetCurrentProcId();
  return base::StringPrintf("%d.%u.%d",
      process_id,
      g_last_id.GetNext(),
      base::RandInt(0, std::numeric_limits<int32>::max()));
}

}  // namespace IPC

