// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/client/visibility_client.h"

#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_property.h"

DECLARE_WINDOW_PROPERTY_TYPE(aura::client::VisibilityClient*)

namespace aura {
namespace client {

// A property key to store a client that handles window visibility changes.
DEFINE_LOCAL_WINDOW_PROPERTY_KEY(
    VisibilityClient*, kWindowVisibilityClientKey, NULL);


void SetVisibilityClient(Window* window, VisibilityClient* client) {
  window->SetProperty(kWindowVisibilityClientKey, client);
}

VisibilityClient* GetVisibilityClient(Window* window) {
  VisibilityClient* visibility_client = NULL;
  aura::Window* current = window;
  do {
    visibility_client = current->GetProperty(kWindowVisibilityClientKey);
    current = current->parent();
  } while (current && !visibility_client);
  return visibility_client;
}

}  // namespace client
}  // namespace aura
