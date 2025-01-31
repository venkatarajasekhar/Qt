// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_REGISTRAR_H_
#define CC_ANIMATION_ANIMATION_REGISTRAR_H_

#include "base/containers/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "cc/base/cc_export.h"

namespace cc {

class LayerAnimationController;

class CC_EXPORT AnimationRegistrar {
 public:
  typedef base::hash_map<int, LayerAnimationController*> AnimationControllerMap;

  static scoped_ptr<AnimationRegistrar> Create() {
    return make_scoped_ptr(new AnimationRegistrar());
  }

  virtual ~AnimationRegistrar();

  // If an animation has been registered for the given id, return it. Otherwise
  // creates a new one and returns a scoped_refptr to it.
  scoped_refptr<LayerAnimationController> GetAnimationControllerForId(int id);

  // Registers the given animation controller as active. An active animation
  // controller is one that has a running animation that needs to be ticked.
  void DidActivateAnimationController(LayerAnimationController* controller);

  // Unregisters the given animation controller. When this happens, the
  // animation controller will no longer be ticked (since it's not active). It
  // is not an error to call this function with a deactivated controller.
  void DidDeactivateAnimationController(LayerAnimationController* controller);

  // Registers the given controller as alive.
  void RegisterAnimationController(LayerAnimationController* controller);

  // Unregisters the given controller as alive.
  void UnregisterAnimationController(LayerAnimationController* controller);

  const AnimationControllerMap& active_animation_controllers() const {
    return active_animation_controllers_;
  }

  const AnimationControllerMap& all_animation_controllers() const {
    return all_animation_controllers_;
  }

 private:
  AnimationRegistrar();

  AnimationControllerMap active_animation_controllers_;
  AnimationControllerMap all_animation_controllers_;

  DISALLOW_COPY_AND_ASSIGN(AnimationRegistrar);
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_REGISTRAR_H_
