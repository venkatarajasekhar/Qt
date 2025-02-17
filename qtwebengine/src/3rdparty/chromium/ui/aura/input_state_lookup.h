// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_INPUT_STATE_LOOKUP_H_
#define UI_AURA_INPUT_STATE_LOOKUP_H_

#include "base/memory/scoped_ptr.h"
#include "ui/aura/aura_export.h"

namespace aura {

// InputStateLookup is used to obtain the state of input devices.
class AURA_EXPORT InputStateLookup {
 public:
  virtual ~InputStateLookup() {}

  // Creates the platform specific InputStateLookup. May return NULL.
  static scoped_ptr<InputStateLookup> Create();

  // Returns true if any mouse button is down.
  virtual bool IsMouseButtonDown() const = 0;
};

}  // namespace aura

#endif  // UI_AURA_INPUT_STATE_LOOKUP_H_
