// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_TEXTFIELD_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_TEXTFIELD_EXAMPLE_H_

#include <string>

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/examples/example_base.h"

namespace views {

class LabelButton;

namespace examples {

// TextfieldExample mimics login screen.
class VIEWS_EXAMPLES_EXPORT TextfieldExample : public ExampleBase,
                                               public TextfieldController,
                                               public ButtonListener {
 public:
  TextfieldExample();
  virtual ~TextfieldExample();

  // ExampleBase:
  virtual void CreateExampleView(View* container) OVERRIDE;

 private:
  // TextfieldController:
  virtual void ContentsChanged(Textfield* sender,
                               const base::string16& new_contents) OVERRIDE;
  virtual bool HandleKeyEvent(Textfield* sender,
                              const ui::KeyEvent& key_event) OVERRIDE;
  virtual bool HandleMouseEvent(Textfield* sender,
                                const ui::MouseEvent& mouse_event) OVERRIDE;

  // ButtonListener:
  virtual void ButtonPressed(Button* sender, const ui::Event& event) OVERRIDE;

  // Textfields for name and password.
  Textfield* name_;
  Textfield* password_;
  Textfield* read_only_;

  // Various buttons to control textfield.
  LabelButton* show_password_;
  LabelButton* clear_all_;
  LabelButton* append_;
  LabelButton* set_;
  LabelButton* set_style_;

  DISALLOW_COPY_AND_ASSIGN(TextfieldExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_TEXTFIELD_EXAMPLE_H_
