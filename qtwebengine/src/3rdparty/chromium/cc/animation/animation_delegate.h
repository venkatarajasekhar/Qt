// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_DELEGATE_H_
#define CC_ANIMATION_ANIMATION_DELEGATE_H_

#include "base/time/time.h"
#include "cc/animation/animation.h"

namespace cc {

class AnimationDelegate {
 public:
  virtual void NotifyAnimationStarted(
      base::TimeTicks monotonic_time,
      Animation::TargetProperty target_property) = 0;
  virtual void NotifyAnimationFinished(
      base::TimeTicks monotonic_time,
      Animation::TargetProperty target_property) = 0;

 protected:
  virtual ~AnimationDelegate() {}
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_DELEGATE_H_
