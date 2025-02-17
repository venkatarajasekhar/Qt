// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_VIEWS_SWITCHES_H_
#define UI_VIEWS_VIEWS_SWITCHES_H_

#include "ui/views/views_export.h"

#include "build/build_config.h"

namespace views {
namespace switches {

// Please keep alphabetized.
VIEWS_EXPORT extern const char kDisableViewsRectBasedTargeting[];

#if defined(USE_X11) && !defined(OS_CHROMEOS)
VIEWS_EXPORT extern const char kEnableTransparentVisuals[];
#endif

// Returns true if rect-based targeting in views should be used.
VIEWS_EXPORT bool IsRectBasedTargetingEnabled();

}  // namespace switches
}  // namespace views

#endif  // UI_VIEWS_VIEWS_SWITCHES_H_
