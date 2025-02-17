/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebGeolocationPermissionRequest_h
#define WebGeolocationPermissionRequest_h

#include "../platform/WebCommon.h"
#include "../platform/WebPrivatePtr.h"

namespace WebCore {
class Geolocation;
}

namespace blink {
class WebSecurityOrigin;

// WebGeolocationPermissionRequest encapsulates a WebCore Geolocation object and represents
// a request from WebCore for permission to be determined for that Geolocation object.
// The underlying Geolocation object is guaranteed to be valid until the invocation of
// either  WebGeolocationPermissionRequest::setIsAllowed (request complete) or
// WebGeolocationClient::cancelPermissionRequest (request cancelled).
class WebGeolocationPermissionRequest {
public:
    ~WebGeolocationPermissionRequest() { reset(); }
    BLINK_EXPORT WebSecurityOrigin securityOrigin() const;
    BLINK_EXPORT void setIsAllowed(bool);

#if BLINK_IMPLEMENTATION
    WebGeolocationPermissionRequest(WebCore::Geolocation*);
    WebCore::Geolocation* geolocation() const { return m_private.get(); }
#endif

private:
    BLINK_EXPORT void reset();

    WebPrivatePtr<WebCore::Geolocation> m_private;
};
}

#endif // WebGeolocationPermissionRequest_h
