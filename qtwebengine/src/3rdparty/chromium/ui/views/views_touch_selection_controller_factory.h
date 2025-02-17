// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_UI_VIEWS_VIEWS_TOUCH_SELECTION_CONTROLLER_FACTORY_H_
#define UI_UI_VIEWS_VIEWS_TOUCH_SELECTION_CONTROLLER_FACTORY_H_

#include "ui/base/touch/touch_editing_controller.h"
#include "ui/views/views_export.h"

namespace views {

class VIEWS_EXPORT ViewsTouchSelectionControllerFactory
    : public ui::TouchSelectionControllerFactory {
 public:
  ViewsTouchSelectionControllerFactory();

  // Overridden from ui::TouchSelectionControllerFactory.
  virtual ui::TouchSelectionController* create(
      ui::TouchEditable* client_view) OVERRIDE;
};

}  // namespace views

#endif  // UI_UI_VIEWS_TOUCHUI_TOUCH_SELECTION_CONTROLLER_IMPL_H_
