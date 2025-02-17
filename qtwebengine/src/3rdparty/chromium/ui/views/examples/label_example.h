// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_LABEL_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_LABEL_EXAMPLE_H_

#include "base/macros.h"
#include "ui/views/examples/example_base.h"

namespace views {
namespace examples {

class VIEWS_EXAMPLES_EXPORT LabelExample : public ExampleBase {
 public:
  LabelExample();
  virtual ~LabelExample();

  // ExampleBase:
  virtual void CreateExampleView(View* container) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(LabelExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_LABEL_EXAMPLE_H_
