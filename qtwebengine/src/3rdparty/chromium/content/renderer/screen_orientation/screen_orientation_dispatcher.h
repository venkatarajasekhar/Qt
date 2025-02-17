// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_H_
#define CONTENT_RENDERER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_H_

#include "base/compiler_specific.h"
#include "base/id_map.h"
#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/platform/WebLockOrientationCallback.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationClient.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationLockType.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationType.h"

namespace content {

class RenderFrame;

// ScreenOrientationDispatcher implements the WebScreenOrientationClient API
// which handles screen lock. It sends lock (or unlock) requests to the browser
// process and listens for responses and let Blink know about the result of the
// request via WebLockOrientationCallback.
class CONTENT_EXPORT ScreenOrientationDispatcher :
    public RenderFrameObserver,
    NON_EXPORTED_BASE(public blink::WebScreenOrientationClient) {
 public:
  explicit ScreenOrientationDispatcher(RenderFrame* render_frame);
  virtual ~ScreenOrientationDispatcher();

 private:
  friend class ScreenOrientationDispatcherTest;

  // RenderFrameObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // blink::WebScreenOrientationClient implementation.
  virtual void lockOrientation(
      blink::WebScreenOrientationLockType orientation,
      blink::WebLockOrientationCallback* callback) OVERRIDE;
  virtual void unlockOrientation() OVERRIDE;

  void OnLockSuccess(int request_id,
                     unsigned angle,
                     blink::WebScreenOrientationType orientation);
  void OnLockError(int request_id,
                   blink::WebLockOrientationCallback::ErrorType error);

  void CancelPendingLocks();

  // The pending_callbacks_ map is mostly meant to have a unique ID to associate
  // with every callback going trough the dispatcher. The map will own the
  // pointer in the sense that it will destroy it when Remove() will be called.
  // Furthermore, we only expect to have one callback at a time in this map,
  // which is what IDMap was designed for.
  typedef IDMap<blink::WebLockOrientationCallback, IDMapOwnPointer> CallbackMap;
  CallbackMap pending_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationDispatcher);
};

}  // namespace content

#endif // CONTENT_RENDERER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_H_
