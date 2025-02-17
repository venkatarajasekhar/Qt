// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MOUSE_WATCHER_H_
#define UI_VIEWS_MOUSE_WATCHER_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "ui/gfx/insets.h"
#include "ui/views/views_export.h"

namespace gfx {
class Point;
}

namespace views {

// MouseWatcherListener is notified when the mouse moves outside the host.
class VIEWS_EXPORT MouseWatcherListener {
 public:
  virtual void MouseMovedOutOfHost() = 0;

 protected:
  virtual ~MouseWatcherListener();
};

// The MouseWatcherHost determines what region is to be monitored.
class VIEWS_EXPORT MouseWatcherHost {
 public:
  // The MouseEventType can be used as a hint.
  enum MouseEventType {
    // The mouse moved within the window which was current when the MouseWatcher
    // was created.
    MOUSE_MOVE,
    // The mouse moved exited the window which was current when the MouseWatcher
    // was created.
    MOUSE_EXIT
  };

  virtual ~MouseWatcherHost();

  // Return false when the mouse has moved outside the monitored region.
  virtual bool Contains(const gfx::Point& screen_point,
                        MouseEventType type) = 0;
};

// MouseWatcher is used to watch mouse movement and notify its listener when the
// mouse moves outside the bounds of a MouseWatcherHost.
class VIEWS_EXPORT MouseWatcher {
 public:
  // Creates a new MouseWatcher. The |listener| will be notified when the |host|
  // determines that the mouse has moved outside its monitored region.
  // |host| will be owned by the watcher and deleted upon completion.
  MouseWatcher(MouseWatcherHost* host, MouseWatcherListener* listener);
  ~MouseWatcher();

  // Sets the amount to delay before notifying the listener when the mouse exits
  // the host by way of going to another window.
  void set_notify_on_exit_time(base::TimeDelta time) {
    notify_on_exit_time_ = time;
  }

  // Starts watching mouse movements. When the mouse moves outside the bounds of
  // the host the listener is notified. |Start| may be invoked any number of
  // times. If the mouse moves outside the bounds of the host the listener is
  // notified and watcher stops watching events.
  void Start();

  // Stops watching mouse events.
  void Stop();

 private:
  class Observer;

  // Are we currently observing events?
  bool is_observing() const { return observer_.get() != NULL; }

  // Notifies the listener and stops watching events.
  void NotifyListener();

  // Host we're listening for events over.
  scoped_ptr<MouseWatcherHost> host_;

  // Our listener.
  MouseWatcherListener* listener_;

  // Does the actual work of listening for mouse events.
  scoped_ptr<Observer> observer_;

  // See description above setter.
  base::TimeDelta notify_on_exit_time_;

  DISALLOW_COPY_AND_ASSIGN(MouseWatcher);
};

}  // namespace views

#endif  // UI_VIEWS_MOUSE_WATCHER_H_
