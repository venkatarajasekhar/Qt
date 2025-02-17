// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_PUBLIC_ACTIVATION_DELEGATE_H_
#define UI_WM_PUBLIC_ACTIVATION_DELEGATE_H_

#include "ui/aura/aura_export.h"

namespace ui {
class Event;
}

namespace aura {
class Window;
namespace client {

// An interface implemented by an object that configures and responds to changes
// to a window's activation state.
class AURA_EXPORT ActivationDelegate {
 public:
  // Returns true if the window should be activated.
  virtual bool ShouldActivate() const = 0;

 protected:
  virtual ~ActivationDelegate() {}
};

// Sets/Gets the ActivationDelegate on the Window. No ownership changes.
AURA_EXPORT void SetActivationDelegate(Window* window,
                                       ActivationDelegate* delegate);
AURA_EXPORT ActivationDelegate* GetActivationDelegate(Window* window);

}  // namespace client
}  // namespace aura

#endif  // UI_WM_PUBLIC_ACTIVATION_DELEGATE_H_
