// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_NOTIFICATION_BLOCKER_H_
#define UI_MESSAGE_CENTER_NOTIFICATION_BLOCKER_H_

#include "base/observer_list.h"
#include "ui/message_center/message_center_export.h"
#include "ui/message_center/notification.h"

namespace message_center {
class MessageCenter;

// NotificationBlocker manages the availability of notifications based on the
// current system status. Each NotificationBlocker implementation covers a
// single state such as screen lock or fullscreen.
class MESSAGE_CENTER_EXPORT NotificationBlocker {
 public:
  class Observer {
   public:
    virtual void OnBlockingStateChanged(NotificationBlocker* blocker) = 0;
  };

  explicit NotificationBlocker(MessageCenter* message_center);
  virtual ~NotificationBlocker();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Checks the current state and updates the availability.
  virtual void CheckState() {}

  // Returns true when notifications from |notifier_id| should appear in the
  // message center. Default returns true always.
  virtual bool ShouldShowNotification(const NotifierId& notifier_id) const;

  // Returns true when notifications from |notifier_id| should be shown as
  // popups on screen.  If it's false, those notifications should be queued.
  // When a blocker starts returning false for a notification which is already
  // shown as a popup, the notification should be closed as a popup immediately.
  virtual bool ShouldShowNotificationAsPopup(
      const NotifierId& notifier_id) const = 0;

 protected:
  MessageCenter* message_center() { return message_center_; }
  void NotifyBlockingStateChanged();

 private:
  ObserverList<Observer> observers_;
  MessageCenter* message_center_;  // weak
};

typedef std::vector<NotificationBlocker*> NotificationBlockers;

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_NOTIFICATION_BLOCKER_H_
