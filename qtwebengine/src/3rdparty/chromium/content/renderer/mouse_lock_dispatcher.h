// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MOUSE_LOCK_DISPATCHER_H_
#define CONTENT_RENDERER_MOUSE_LOCK_DISPATCHER_H_

#include "base/basictypes.h"
#include "content/common/content_export.h"

namespace blink {
class WebMouseEvent;
}  // namespace blink

namespace content {

class CONTENT_EXPORT MouseLockDispatcher {
 public:
  MouseLockDispatcher();
  virtual ~MouseLockDispatcher();

  class LockTarget {
   public:
    virtual ~LockTarget() {}
    // A mouse lock request was pending and this reports success or failure.
    virtual void OnLockMouseACK(bool succeeded) = 0;
    // A mouse lock was in place, but has been lost.
    virtual void OnMouseLockLost() = 0;
    // A mouse lock is enabled and mouse events are being delievered.
    virtual bool HandleMouseLockedInputEvent(
        const blink::WebMouseEvent& event) = 0;
  };

  // Locks the mouse to the |target|. If true is returned, an asynchronous
  // response to target->OnLockMouseACK() will follow.
  bool LockMouse(LockTarget* target);
  // Request to unlock the mouse. An asynchronous response to
  // target->OnMouseLockLost() will follow.
  void UnlockMouse(LockTarget* target);
  // Clears out the reference to the |target| because it has or is being
  // destroyed. Unlocks if locked. The pointer will not be accessed.
  void OnLockTargetDestroyed(LockTarget* target);
  bool IsMouseLockedTo(LockTarget* target);

  // Allow lock target to consumed a mouse event, if it does return true.
  bool WillHandleMouseEvent(const blink::WebMouseEvent& event);

  // Subclasses or users have to call these methods to report mouse lock events
  // from the browser.
  void OnLockMouseACK(bool succeeded);
  void OnMouseLockLost();

 protected:
  // Subclasses must implement these methods to send mouse lock requests to the
  // browser.
  virtual void SendLockMouseRequest(bool unlocked_by_target) = 0;
  virtual void SendUnlockMouseRequest() = 0;

 private:
  bool MouseLockedOrPendingAction() const {
    return mouse_locked_ || pending_lock_request_ || pending_unlock_request_;
  }

  bool mouse_locked_;
  // If both |pending_lock_request_| and |pending_unlock_request_| are true,
  // it means a lock request was sent before an unlock request and we haven't
  // received responses for them. The logic in LockMouse() makes sure that a
  // lock request won't be sent when there is a pending unlock request.
  bool pending_lock_request_;
  bool pending_unlock_request_;

  // Used when locking to indicate when a target application has voluntarily
  // unlocked and desires to relock the mouse. If the mouse is unlocked due
  // to ESC being pressed by the user, this will be false
  bool unlocked_by_target_;

  // |target_| is the pending or current owner of mouse lock. We retain a non
  // owning reference here that must be cleared by |OnLockTargetDestroyed|
  // when it is destroyed.
  LockTarget* target_;

  DISALLOW_COPY_AND_ASSIGN(MouseLockDispatcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MOUSE_LOCK_DISPATCHER_H_
