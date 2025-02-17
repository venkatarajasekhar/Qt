// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_EVDEV_CURSOR_DELEGATE_EVDEV_H_
#define UI_EVENTS_OZONE_EVDEV_CURSOR_DELEGATE_EVDEV_H_

#include "ui/events/ozone/evdev/events_ozone_evdev_export.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Vector2dF;
}

namespace ui {

class EVENTS_OZONE_EVDEV_EXPORT CursorDelegateEvdev {
 public:
  // Move the cursor.
  virtual void MoveCursor(const gfx::Vector2dF& delta) = 0;
  virtual void MoveCursorTo(gfx::AcceleratedWidget widget,
                            const gfx::PointF& location) = 0;

  // Location in window.
  virtual gfx::PointF location() = 0;
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_CURSOR_DELEGATE_EVDEV_H_
