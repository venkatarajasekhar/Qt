// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/debug/paint_time_counter.h"

namespace cc {

// static
scoped_ptr<PaintTimeCounter> PaintTimeCounter::Create() {
  return make_scoped_ptr(new PaintTimeCounter());
}

PaintTimeCounter::PaintTimeCounter()
  : can_save_paint_time_delta_(false) {
}

void PaintTimeCounter::SavePaintTime(const base::TimeDelta& total_paint_time) {
  if (can_save_paint_time_delta_) {
    base::TimeDelta paint_time = total_paint_time - last_total_paint_time_;
    ring_buffer_.SaveToBuffer(paint_time);
  }

  last_total_paint_time_ = total_paint_time;
  can_save_paint_time_delta_ = true;
}

void PaintTimeCounter::GetMinAndMaxPaintTime(base::TimeDelta* min,
                                             base::TimeDelta* max) const {
  *min = base::TimeDelta::FromDays(1);
  *max = base::TimeDelta();

  for (RingBufferType::Iterator it = ring_buffer_.Begin(); it; ++it) {
    const base::TimeDelta paint_time = **it;

    if (paint_time < *min)
      *min = paint_time;
    if (paint_time > *max)
      *max = paint_time;
  }

  if (*min > *max)
    *min = *max;
}

void PaintTimeCounter::ClearHistory() {
  ring_buffer_.Clear();
  can_save_paint_time_delta_ = false;
}

}  // namespace cc
