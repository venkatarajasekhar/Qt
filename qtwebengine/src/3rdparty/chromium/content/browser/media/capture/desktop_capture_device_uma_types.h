// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_CAPTURE_DEVICE_UMA_TYPES_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_CAPTURE_DEVICE_UMA_TYPES_H_

namespace content {

// This enum must be kept in-sync with DesktopCaptureCounters defined in
// histograms.xml. New fields should be added right before
// DESKTOP_CAPTURE_COUNTER_BOUNDARY.
enum DesktopCaptureCounters {
  SCREEN_CAPTURER_CREATED,
  WINDOW_CATPTURER_CREATED,
  FIRST_SCREEN_CAPTURE_SUCCEEDED,
  FIRST_SCREEN_CAPTURE_FAILED,
  FIRST_WINDOW_CAPTURE_SUCCEEDED,
  FIRST_WINDOW_CAPTURE_FAILED,
  DESKTOP_CAPTURE_COUNTER_BOUNDARY
};

extern const char kUmaScreenCaptureTime[];
extern const char kUmaWindowCaptureTime[];

void IncrementDesktopCaptureCounter(DesktopCaptureCounters counter);

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_CAPTURE_DEVICE_UMA_TYPES_H_
