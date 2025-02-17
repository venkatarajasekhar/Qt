// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIN_FULLSCREEN_HANDLER_H_
#define UI_VIEWS_WIN_FULLSCREEN_HANDLER_H_

#include <windows.h>

#include <map>

#include "base/basictypes.h"

namespace gfx {
class Rect;
}

namespace views {

class FullscreenHandler {
 public:
  FullscreenHandler();
  ~FullscreenHandler();

  void set_hwnd(HWND hwnd) { hwnd_ = hwnd; }

  void SetFullscreen(bool fullscreen);
  void SetMetroSnap(bool metro_snap);

  gfx::Rect GetRestoreBounds() const;

  bool fullscreen() const { return fullscreen_; }
  bool metro_snap() const { return metro_snap_; }

 private:
  // Information saved before going into fullscreen mode, used to restore the
  // window afterwards.
  struct SavedWindowInfo {
    bool maximized;
    LONG style;
    LONG ex_style;
    RECT window_rect;
  };

  void SetFullscreenImpl(bool fullscreen, bool for_metro);

  HWND hwnd_;
  bool fullscreen_;
  bool metro_snap_;

  // Saved window information from before entering fullscreen mode.
  // TODO(beng): move to private once GetRestoredBounds() moves onto Widget.
  SavedWindowInfo saved_window_info_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenHandler);
};

}  // namespace views

#endif  // UI_VIEWS_WIN_FULLSCREEN_HANDLER_H_