// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/environment/environment.h"

#include "mojo/environment/default_async_waiter_impl.h"
#include "mojo/environment/default_logger_impl.h"

namespace mojo {

// These methods are intentionally not implemented so that there is a link
// error if someone uses them in a Chromium-environment.
#if 0
Environment::Environment() {
}

Environment::Environment(const MojoAsyncWaiter* default_async_waiter,
                         const MojoLogger* default_logger) {
}

Environment::~Environment() {
}
#endif

// static
const MojoAsyncWaiter* Environment::GetDefaultAsyncWaiter() {
  return internal::GetDefaultAsyncWaiterImpl();
}

// static
const MojoLogger* Environment::GetDefaultLogger() {
  return internal::GetDefaultLoggerImpl();
}

}  // namespace mojo
