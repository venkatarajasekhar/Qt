// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_ROUTER_CONFIG_HELPER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_ROUTER_CONFIG_HELPER_H_

#include "content/browser/renderer_host/input/input_router_impl.h"

namespace content {

// Return an InputRouter configuration with parameters tailored to the current
// platform.
InputRouterImpl::Config GetInputRouterConfigForPlatform();

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_ROUTER_CONFIG_HELPER_H_
