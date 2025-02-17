// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ANIMATION_SCROLL_ANIMATOR_H_
#define UI_VIEWS_ANIMATION_SCROLL_ANIMATOR_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/views/views_export.h"

namespace gfx {
class SlideAnimation;
}

namespace views {

class VIEWS_EXPORT ScrollDelegate {
 public:
  // Returns true if the content was actually scrolled, false otherwise.
  virtual bool OnScroll(float dx, float dy) = 0;

 protected:
  ~ScrollDelegate() {}
};

class VIEWS_EXPORT ScrollAnimator : public gfx::AnimationDelegate {
 public:
  // The ScrollAnimator does not own the delegate. Uses default acceleration.
  explicit ScrollAnimator(ScrollDelegate* delegate);
  virtual ~ScrollAnimator();

  // Use this if you would prefer different acceleration than the default.
  void set_acceleration(float acceleration) { acceleration_ = acceleration; }

  void Start(float velocity_x, float velocity_y);
  void Stop();

  bool is_scrolling() const { return !!animation_.get(); }

 private:
  // Implementation of gfx::AnimationDelegate.
  virtual void AnimationEnded(const gfx::Animation* animation) OVERRIDE;
  virtual void AnimationProgressed(const gfx::Animation* animation) OVERRIDE;
  virtual void AnimationCanceled(const gfx::Animation* animation) OVERRIDE;

  ScrollDelegate* delegate_;

  float velocity_x_;
  float velocity_y_;
  float last_t_;
  float duration_;
  float acceleration_;

  scoped_ptr<gfx::SlideAnimation> animation_;

  DISALLOW_COPY_AND_ASSIGN(ScrollAnimator);
};

}  // namespace views

#endif  // UI_VIEWS_ANIMATION_SCROLL_ANIMATOR_H_
