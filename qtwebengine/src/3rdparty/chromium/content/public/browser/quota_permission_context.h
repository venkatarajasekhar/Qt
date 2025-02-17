// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_QUOTA_PERMISSION_CONTEXT_H_
#define CONTENT_PUBLIC_BROWSER_QUOTA_PERMISSION_CONTEXT_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/public/common/storage_quota_params.h"
#include "webkit/common/quota/quota_types.h"

class GURL;

namespace content {

class QuotaPermissionContext
    : public base::RefCountedThreadSafe<QuotaPermissionContext> {
 public:
  enum QuotaPermissionResponse {
    QUOTA_PERMISSION_RESPONSE_UNKNOWN,
    QUOTA_PERMISSION_RESPONSE_ALLOW,
    QUOTA_PERMISSION_RESPONSE_DISALLOW,
    QUOTA_PERMISSION_RESPONSE_CANCELLED,
  };

  typedef base::Callback<void(QuotaPermissionResponse)> PermissionCallback;

  virtual void RequestQuotaPermission(
      const StorageQuotaParams& params,
      int render_process_id,
      const PermissionCallback& callback) = 0;

 protected:
  friend class base::RefCountedThreadSafe<QuotaPermissionContext>;
  virtual ~QuotaPermissionContext() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_QUOTA_PERMISSION_CONTEXT_H_
