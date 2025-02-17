// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/mock_appcache_service.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"

namespace content {

static void DeferredCallCallback(
    const net::CompletionCallback& callback, int rv) {
  callback.Run(rv);
}

void MockAppCacheService::DeleteAppCachesForOrigin(
    const GURL& origin, const net::CompletionCallback& callback) {
  ++delete_called_count_;
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&DeferredCallCallback,
                 callback,
                 mock_delete_appcaches_for_origin_result_));
}

}  // namespace content
