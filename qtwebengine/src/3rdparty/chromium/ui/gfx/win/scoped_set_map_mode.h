// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_WIN_SCOPED_SET_MAP_MODE_H_
#define UI_GFX_WIN_SCOPED_SET_MAP_MODE_H_

#include <windows.h>

#include "base/basictypes.h"
#include "base/logging.h"

namespace gfx {

// Helper class for setting and restore the map mode on a DC.
class ScopedSetMapMode {
 public:
  ScopedSetMapMode(HDC hdc, int map_mode)
      : hdc_(hdc),
        old_map_mode_(SetMapMode(hdc, map_mode)) {
    DCHECK(hdc_);
    DCHECK_NE(map_mode, 0);
    DCHECK_NE(old_map_mode_, 0);
  }

  ~ScopedSetMapMode() {
    const int mode = SetMapMode(hdc_, old_map_mode_);
    DCHECK_NE(mode, 0);
  }

 private:
  HDC hdc_;
  int old_map_mode_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSetMapMode);
};

}  // namespace gfx

#endif  // UI_GFX_WIN_SCOPED_SET_MAP_MODE_H_
