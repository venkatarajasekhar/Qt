// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Media device IDs come in two flavors: The machine-wide unique ID of
// the device, which is what we use on the browser side, and one-way
// hashes over the unique ID and a security origin, which we provide
// to code on the renderer side as per-security-origin IDs.

#ifndef CONTENT_PUBLIC_BROWSER_MEDIA_DEVICE_ID_H_
#define CONTENT_PUBLIC_BROWSER_MEDIA_DEVICE_ID_H_

#include <string>

#include "content/common/content_export.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/media_stream_request.h"
#include "url/gurl.h"

namespace content {

// Generates a one-way hash of a device's unique ID usable by one
// particular security origin.
CONTENT_EXPORT std::string GetHMACForMediaDeviceID(
    const ResourceContext::SaltCallback& sc,
    const GURL& security_origin,
    const std::string& raw_unique_id);

// Convenience method to check if |device_guid| is an HMAC of
// |raw_device_id| for |security_origin|.
CONTENT_EXPORT bool DoesMediaDeviceIDMatchHMAC(
    const ResourceContext::SaltCallback& sc,
    const GURL& security_origin,
    const std::string& device_guid,
    const std::string& raw_unique_id);

CONTENT_EXPORT bool GetMediaDeviceIDForHMAC(
    MediaStreamType stream_type,
    const ResourceContext::SaltCallback& rc,
    const GURL& security_origin,
    const std::string& source_id,
    std::string* device_id);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_MEDIA_DEVICE_ID_H_
