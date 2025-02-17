// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_DIP_UTIL_H_
#define CONTENT_BROWSER_RENDERER_HOST_DIP_UTIL_H_

#include "content/common/content_export.h"

namespace gfx {
class Point;
class Rect;
class Size;
}  // namespace gfx

namespace content {
class RenderWidgetHostView;

// Returns scale factor of the display nearest to |view|.
// Returns 1.0f if the platform does not support DIP.
CONTENT_EXPORT float GetScaleFactorForView(const RenderWidgetHostView* view);

// Utility functions that convert point/size/rect between DIP and pixel
// coordinate system.
CONTENT_EXPORT gfx::Point ConvertViewPointToDIP(
    const RenderWidgetHostView* view, const gfx::Point& point_in_pixel);
CONTENT_EXPORT gfx::Size ConvertViewSizeToPixel(
    const RenderWidgetHostView* view, const gfx::Size& size_in_dip);
CONTENT_EXPORT gfx::Rect ConvertViewRectToPixel(
    const RenderWidgetHostView* view, const gfx::Rect& rect_in_dip);

CONTENT_EXPORT gfx::Size ConvertSizeToDIP(
    float scale_factor, const gfx::Size& size_in_pixel);
CONTENT_EXPORT gfx::Rect ConvertRectToDIP(
    float scale_factor, const gfx::Rect& rect_in_pixel);
CONTENT_EXPORT gfx::Size ConvertSizeToPixel(
    float scale_factor, const gfx::Size& size_in_pixel);
CONTENT_EXPORT gfx::Rect ConvertRectToPixel(
    float scale_factor, const gfx::Rect& rect_in_dip);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_DIP_UTIL_H_
