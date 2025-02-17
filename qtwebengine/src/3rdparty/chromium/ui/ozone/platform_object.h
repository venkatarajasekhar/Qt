// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_OBJECT_H_
#define UI_OZONE_PLATFORM_OBJECT_H_

#include "base/memory/scoped_ptr.h"

namespace ui {

// Create an instance of platform specific object.
//
// This calls a static constructor function based on the --ozone-platform flag.
//
// For the platform called "foo", PlatformObject<PlatformWidget> will ultimately
// call the function with signature
//
//   Bar* CreatePlatformWidgetFoo();
//
// A definition of this function for each compiled platform must be provided, or
// link errors will result.
//
// To find the right constructor function, this uses static data defined in the
// source file generated by the generate_constructor_list.py.
template <class T>
class PlatformObject {
 public:
  static scoped_ptr<T> Create();
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_OBJECT_H_
