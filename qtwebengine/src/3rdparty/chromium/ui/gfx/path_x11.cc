// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/path_x11.h"

#include <X11/Xlib.h>
#include <X11/Xregion.h>
#include <X11/Xutil.h>

#include "base/memory/scoped_ptr.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/path.h"

namespace gfx {

Region CreateRegionFromSkRegion(const SkRegion& region) {
  Region result = XCreateRegion();

  for (SkRegion::Iterator i(region); !i.done(); i.next()) {
    XRectangle rect;
    rect.x = i.rect().x();
    rect.y = i.rect().y();
    rect.width = i.rect().width();
    rect.height = i.rect().height();
    XUnionRectWithRegion(&rect, result, result);
  }

  return result;
}

Region CreateRegionFromSkPath(const SkPath& path) {
  int point_count = path.getPoints(NULL, 0);
  scoped_ptr<SkPoint[]> points(new SkPoint[point_count]);
  path.getPoints(points.get(), point_count);
  scoped_ptr<XPoint[]> x11_points(new XPoint[point_count]);
  for (int i = 0; i < point_count; ++i) {
    x11_points[i].x = SkScalarRoundToInt(points[i].fX);
    x11_points[i].y = SkScalarRoundToInt(points[i].fY);
  }

  return XPolygonRegion(x11_points.get(), point_count, EvenOddRule);
}

}  // namespace gfx
