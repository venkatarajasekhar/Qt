// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/client/focus_change_observer.h"

#include "ui/aura/window.h"
#include "ui/aura/window_property.h"

DECLARE_WINDOW_PROPERTY_TYPE(aura::client::FocusChangeObserver*)

namespace aura {
namespace client {

DEFINE_LOCAL_WINDOW_PROPERTY_KEY(
    FocusChangeObserver*, kFocusChangeObserverKey, NULL);

FocusChangeObserver* GetFocusChangeObserver(Window* window) {
  return window ? window->GetProperty(kFocusChangeObserverKey) : NULL;
}

void SetFocusChangeObserver(Window* window,
                            FocusChangeObserver* focus_change_observer) {
  window->SetProperty(kFocusChangeObserverKey, focus_change_observer);
}

}  // namespace client
}  // namespace aura
