// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_ACCELERATORS_PLATFORM_ACCELERATOR_COCOA_H_
#define UI_BASE_ACCELERATORS_PLATFORM_ACCELERATOR_COCOA_H_

#include <Foundation/Foundation.h>

#include "base/mac/scoped_nsobject.h"
#include "ui/base/accelerators/platform_accelerator.h"

namespace ui {

// This is a Mac specific class for specifing accelerator keys.
class UI_BASE_EXPORT PlatformAcceleratorCocoa : public PlatformAccelerator {
 public:
  PlatformAcceleratorCocoa();
  PlatformAcceleratorCocoa(NSString* key_code, NSUInteger modifier_mask);
  virtual ~PlatformAcceleratorCocoa();

  // PlatformAccelerator:
  virtual scoped_ptr<PlatformAccelerator> CreateCopy() const OVERRIDE;
  virtual bool Equals(const PlatformAccelerator& rhs) const OVERRIDE;

  // The keyEquivalent of the NSMenuItem associated with the accelerator.
  NSString* characters() const { return characters_.get(); }
  // The keyEquivalentModifierMask of the NSMenuItem associated with the
  // accelerator.
  NSUInteger modifier_mask() const { return modifier_mask_; }

 private:
  // String of characters for the key equivalent.
  base::scoped_nsobject<NSString> characters_;
  NSUInteger modifier_mask_;

  DISALLOW_COPY_AND_ASSIGN(PlatformAcceleratorCocoa);
};

}  // namespace ui

#endif  // UI_BASE_ACCELERATORS_PLATFORM_ACCELERATOR_COCOA_H_
