// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_SCROLLBAR_ANIMATION_CONTROLLER_LINEAR_FADE_H_
#define CC_ANIMATION_SCROLLBAR_ANIMATION_CONTROLLER_LINEAR_FADE_H_

#include "base/memory/scoped_ptr.h"
#include "cc/animation/scrollbar_animation_controller.h"
#include "cc/base/cc_export.h"

namespace cc {
class LayerImpl;

class CC_EXPORT ScrollbarAnimationControllerLinearFade
    : public ScrollbarAnimationController {
 public:
  static scoped_ptr<ScrollbarAnimationControllerLinearFade> Create(
      LayerImpl* scroll_layer,
      ScrollbarAnimationControllerClient* client,
      base::TimeDelta delay_before_starting,
      base::TimeDelta duration);

  virtual ~ScrollbarAnimationControllerLinearFade();

  virtual void DidScrollUpdate() OVERRIDE;

 protected:
  ScrollbarAnimationControllerLinearFade(
      LayerImpl* scroll_layer,
      ScrollbarAnimationControllerClient* client,
      base::TimeDelta delay_before_starting,
      base::TimeDelta duration);

  virtual void RunAnimationFrame(float progress) OVERRIDE;

 private:
  float OpacityAtTime(base::TimeTicks now) const;
  void ApplyOpacityToScrollbars(float opacity);

  LayerImpl* scroll_layer_;

  DISALLOW_COPY_AND_ASSIGN(ScrollbarAnimationControllerLinearFade);
};

}  // namespace cc

#endif  // CC_ANIMATION_SCROLLBAR_ANIMATION_CONTROLLER_LINEAR_FADE_H_
