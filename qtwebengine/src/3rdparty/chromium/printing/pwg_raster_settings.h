// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_BITMAP_TRANSFORM_SETTINGS_H_
#define PRINTING_BITMAP_TRANSFORM_SETTINGS_H_

#include "base/tuple.h"
#include "ipc/ipc_param_traits.h"
#include "printing/printing_export.h"
#include "ui/gfx/rect.h"

namespace printing {

enum PwgRasterTransformType {
  TRANSFORM_NORMAL,
  TRANSFORM_ROTATE_180,
  TRANSFORM_FLIP_HORIZONTAL,
  TRANSFORM_FLIP_VERTICAL
};

struct PwgRasterSettings {
  // How to transform odd-numbered pages.
  PwgRasterTransformType odd_page_transform;
  // Rotate all pages (on top of odd-numbered page transform).
  bool rotate_all_pages;
  // Rasterize pages in reverse order.
  bool reverse_page_order;
};

}  // namespace printing

#endif  // PRINTING_BITMAP_TRANSFORM_SETTINGS_H_
