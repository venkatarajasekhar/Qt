// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INPUT_DID_OVERSCROLL_PARAMS_H_
#define CONTENT_COMMON_INPUT_DID_OVERSCROLL_PARAMS_H_

#include "ui/gfx/geometry/vector2d_f.h"

namespace content {

struct DidOverscrollParams {
  gfx::Vector2dF accumulated_overscroll;
  gfx::Vector2dF latest_overscroll_delta;
  gfx::Vector2dF current_fling_velocity;
};

}  // namespace content

#endif  // CONTENT_COMMON_INPUT_DID_OVERSCROLL_PARAMS_H_
