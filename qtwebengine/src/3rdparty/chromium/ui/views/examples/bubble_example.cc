// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/bubble_example.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

using base::ASCIIToUTF16;

namespace views {
namespace examples {

namespace {

SkColor colors[] = { SK_ColorWHITE, SK_ColorGRAY, SK_ColorCYAN, 0xFFC1B1E1 };

BubbleBorder::Arrow arrows[] = {
    BubbleBorder::TOP_LEFT, BubbleBorder::TOP_CENTER,
    BubbleBorder::TOP_RIGHT, BubbleBorder::RIGHT_TOP,
    BubbleBorder::RIGHT_CENTER, BubbleBorder::RIGHT_BOTTOM,
    BubbleBorder::BOTTOM_RIGHT, BubbleBorder::BOTTOM_CENTER,
    BubbleBorder::BOTTOM_LEFT, BubbleBorder::LEFT_BOTTOM,
    BubbleBorder::LEFT_CENTER, BubbleBorder::LEFT_TOP };

base::string16 GetArrowName(BubbleBorder::Arrow arrow) {
  switch (arrow) {
    case BubbleBorder::TOP_LEFT:      return ASCIIToUTF16("TOP_LEFT");
    case BubbleBorder::TOP_RIGHT:     return ASCIIToUTF16("TOP_RIGHT");
    case BubbleBorder::BOTTOM_LEFT:   return ASCIIToUTF16("BOTTOM_LEFT");
    case BubbleBorder::BOTTOM_RIGHT:  return ASCIIToUTF16("BOTTOM_RIGHT");
    case BubbleBorder::LEFT_TOP:      return ASCIIToUTF16("LEFT_TOP");
    case BubbleBorder::RIGHT_TOP:     return ASCIIToUTF16("RIGHT_TOP");
    case BubbleBorder::LEFT_BOTTOM:   return ASCIIToUTF16("LEFT_BOTTOM");
    case BubbleBorder::RIGHT_BOTTOM:  return ASCIIToUTF16("RIGHT_BOTTOM");
    case BubbleBorder::TOP_CENTER:    return ASCIIToUTF16("TOP_CENTER");
    case BubbleBorder::BOTTOM_CENTER: return ASCIIToUTF16("BOTTOM_CENTER");
    case BubbleBorder::LEFT_CENTER:   return ASCIIToUTF16("LEFT_CENTER");
    case BubbleBorder::RIGHT_CENTER:  return ASCIIToUTF16("RIGHT_CENTER");
    case BubbleBorder::NONE:          return ASCIIToUTF16("NONE");
    case BubbleBorder::FLOAT:         return ASCIIToUTF16("FLOAT");
  }
  return ASCIIToUTF16("INVALID");
}

class ExampleBubble : public BubbleDelegateView {
 public:
   ExampleBubble(View* anchor, BubbleBorder::Arrow arrow)
       : BubbleDelegateView(anchor, arrow) {}

 protected:
  virtual void Init() OVERRIDE {
    SetLayoutManager(new BoxLayout(BoxLayout::kVertical, 50, 50, 0));
    AddChildView(new Label(GetArrowName(arrow())));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExampleBubble);
};

}  // namespace

BubbleExample::BubbleExample() : ExampleBase("Bubble") {}

BubbleExample::~BubbleExample() {}

void BubbleExample::CreateExampleView(View* container) {
  PrintStatus("Click with optional modifiers: [Ctrl] for set_arrow(NONE), "
     "[Alt] for set_arrow(FLOAT), or [Shift] to reverse the arrow iteration.");
  container->SetLayoutManager(new BoxLayout(BoxLayout::kHorizontal, 0, 0, 10));
  no_shadow_ = new LabelButton(this, ASCIIToUTF16("No Shadow"));
  container->AddChildView(no_shadow_);
  big_shadow_ = new LabelButton(this, ASCIIToUTF16("Big Shadow"));
  container->AddChildView(big_shadow_);
  small_shadow_ = new LabelButton(this, ASCIIToUTF16("Small Shadow"));
  container->AddChildView(small_shadow_);
  align_to_edge_ = new LabelButton(this, ASCIIToUTF16("Align To Edge"));
  container->AddChildView(align_to_edge_);
  persistent_ = new LabelButton(this, ASCIIToUTF16("Persistent"));
  container->AddChildView(persistent_);
}

void BubbleExample::ButtonPressed(Button* sender, const ui::Event& event) {
  static int arrow_index = 0, color_index = 0;
  static const int count = arraysize(arrows);
  arrow_index = (arrow_index + count + (event.IsShiftDown() ? -1 : 1)) % count;
  BubbleBorder::Arrow arrow = arrows[arrow_index];
  if (event.IsControlDown())
    arrow = BubbleBorder::NONE;
  else if (event.IsAltDown())
    arrow = BubbleBorder::FLOAT;

  ExampleBubble* bubble = new ExampleBubble(sender, arrow);
  bubble->set_color(colors[(color_index++) % arraysize(colors)]);

  if (sender == no_shadow_)
    bubble->set_shadow(BubbleBorder::NO_SHADOW);
  else if (sender == big_shadow_)
    bubble->set_shadow(BubbleBorder::BIG_SHADOW);
  else if (sender == small_shadow_)
    bubble->set_shadow(BubbleBorder::SMALL_SHADOW);

  if (sender == persistent_)
    bubble->set_close_on_deactivate(false);

  BubbleDelegateView::CreateBubble(bubble);
  if (sender == align_to_edge_)
    bubble->SetAlignment(BubbleBorder::ALIGN_EDGE_TO_ANCHOR_EDGE);

  bubble->GetWidget()->Show();
}

}  // namespace examples
}  // namespace views
