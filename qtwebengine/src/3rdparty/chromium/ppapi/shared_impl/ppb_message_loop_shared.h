// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_PPB_MESSAGE_LOOP_SHARED_H_
#define PPAPI_SHARED_IMPL_PPB_MESSAGE_LOOP_SHARED_H_

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/location.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/thunk/ppb_message_loop_api.h"

namespace base {
class MessageLoopProxy;
}

namespace tracked_objects {
class Location;
}

namespace ppapi {

// MessageLoopShared doesn't really do anything interesting. It exists so that
// shared code (in particular, TrackedCallback) can keep a pointer to a
// MessageLoop resource. In the host side, there is not a concrete class that
// implements this. So pointers to MessageLoopShared can only really be valid
// on the plugin side.
class PPAPI_SHARED_EXPORT MessageLoopShared
    : public Resource,
      public thunk::PPB_MessageLoop_API {
 public:
  explicit MessageLoopShared(PP_Instance instance);
  // Construct the one MessageLoopShared for the main thread. This must be
  // invoked on the main thread.
  struct ForMainThread {};
  explicit MessageLoopShared(ForMainThread);
  virtual ~MessageLoopShared();

  // Handles posting to the message loop if there is one, or the pending queue
  // if there isn't.
  // NOTE: The given closure will be run *WITHOUT* acquiring the Proxy lock.
  //       This only makes sense for user code and completely thread-safe
  //       proxy operations (e.g., MessageLoop::QuitClosure).
  virtual void PostClosure(const tracked_objects::Location& from_here,
                           const base::Closure& closure,
                           int64 delay_ms) = 0;

  virtual base::MessageLoopProxy* GetMessageLoopProxy() = 0;

  DISALLOW_COPY_AND_ASSIGN(MessageLoopShared);
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_PPB_MESSAGE_LOOP_SHARED_H_
