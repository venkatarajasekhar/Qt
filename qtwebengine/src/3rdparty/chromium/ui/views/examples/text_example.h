// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_TEXT_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_TEXT_EXAMPLE_H_

#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/examples/example_base.h"

namespace views {
class Checkbox;
class GridLayout;

namespace examples {

class ExampleComboboxModel;

class VIEWS_EXAMPLES_EXPORT TextExample : public ExampleBase,
                                          public ButtonListener,
                                          public ComboboxListener {
 public:
  TextExample();
  virtual ~TextExample();

  // ExampleBase:
  virtual void CreateExampleView(View* container) OVERRIDE;

 private:
  // Creates and adds a check box to the layout.
  Checkbox* AddCheckbox(GridLayout* layout, const char* name);

  // Creates and adds a combobox to the layout.
  Combobox* AddCombobox(GridLayout* layout,
                        const char* name,
                        const char** strings,
                        int count);

  // ButtonListener:
  virtual void ButtonPressed(Button* button, const ui::Event& event) OVERRIDE;

  // ComboboxListener:
  virtual void OnPerformAction(Combobox* combobox) OVERRIDE;

  class TextExampleView;
  // The content of the scroll view.
  TextExampleView* text_view_;

  // Combo box for horizontal text alignment.
  Combobox* h_align_cb_;

  // Combo box for text eliding style.
  Combobox* eliding_cb_;

  // Combo box for ampersand prefix show / hide behavior.
  Combobox* prefix_cb_;

  // Combo box to choose one of the sample texts.
  Combobox* text_cb_;

  // Check box to enable/disable multiline text drawing.
  Checkbox* multiline_checkbox_;

  // Check box to enable/disable character break behavior.
  Checkbox* break_checkbox_;

  // Check box to enable/disable text halo.
  Checkbox* halo_checkbox_;

  // Check box to enable/disable bold style.
  Checkbox* bold_checkbox_;

  // Check box to enable/disable italic style.
  Checkbox* italic_checkbox_;

  // Check box to enable/disable underline style.
  Checkbox* underline_checkbox_;

  // We create a model for each of the combobox, so we need to keep them
  // around until destruction time.
  ScopedVector<ExampleComboboxModel> example_combobox_model_;

  DISALLOW_COPY_AND_ASSIGN(TextExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_TEXT_EXAMPLE_H_
