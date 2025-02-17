// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/desktop_capture_device_uma_types.h"

#include "base/metrics/histogram.h"

namespace content {

const char kUmaScreenCaptureTime[] = "WebRTC.ScreenCaptureTime";
const char kUmaWindowCaptureTime[] = "WebRTC.WindowCaptureTime";

void IncrementDesktopCaptureCounter(DesktopCaptureCounters counter) {
  UMA_HISTOGRAM_ENUMERATION("WebRTC.DesktopCaptureCounters",
                            counter,
                            DESKTOP_CAPTURE_COUNTER_BOUNDARY);
}

}  // namespace content
