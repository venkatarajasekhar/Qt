// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_TOP_CONTROLS_STATE_H_
#define CONTENT_PUBLIC_COMMON_TOP_CONTROLS_STATE_H_

namespace content {

enum TopControlsState {
#define DEFINE_TOP_CONTROLS_STATE(name, value) name = value,
#include "content/public/common/top_controls_state_list.h"
#undef DEFINE_TOP_CONTROLS_STATE
};

} // namespace content

#endif  // CONTENT_PUBLIC_COMMON_TOP_CONTROLS_STATE_H_

