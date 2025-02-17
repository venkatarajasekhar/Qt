/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebRTCStatsResponse_h
#define WebRTCStatsResponse_h

#include "WebCommon.h"
#include "WebPrivatePtr.h"
#include "WebString.h"

namespace WebCore {
class RTCStatsResponseBase;
}

namespace blink {

class WebRTCStatsResponse {
public:
    WebRTCStatsResponse(const WebRTCStatsResponse& other) { assign(other); }
    WebRTCStatsResponse() { }
    ~WebRTCStatsResponse() { reset(); }

    WebRTCStatsResponse& operator=(const WebRTCStatsResponse& other)
    {
        assign(other);
        return *this;
    }

    BLINK_PLATFORM_EXPORT void assign(const WebRTCStatsResponse&);

    BLINK_PLATFORM_EXPORT void reset();

    BLINK_PLATFORM_EXPORT size_t addReport(WebString id, WebString type, double timestamp);
    BLINK_PLATFORM_EXPORT void addStatistic(size_t report, WebString name, WebString value);

#if INSIDE_BLINK
    BLINK_PLATFORM_EXPORT WebRTCStatsResponse(const PassRefPtrWillBeRawPtr<WebCore::RTCStatsResponseBase>&);

    BLINK_PLATFORM_EXPORT operator PassRefPtrWillBeRawPtr<WebCore::RTCStatsResponseBase>() const;
#endif

private:
    WebPrivatePtr<WebCore::RTCStatsResponseBase> m_private;
};

} // namespace blink

#endif // WebRTCStatsResponse_h
