// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebConvertableToTraceFormat_h
#define WebConvertableToTraceFormat_h

#include "WebPrivatePtr.h"
#include "WebString.h"

namespace WebCore {
namespace TraceEvent {
class ConvertableToTraceFormat;
}
}

namespace blink {

class WebConvertableToTraceFormat {
public:
    WebConvertableToTraceFormat() { }
#if INSIDE_BLINK
    WebConvertableToTraceFormat(WebCore::TraceEvent::ConvertableToTraceFormat*);
#endif
    ~WebConvertableToTraceFormat() { reset(); }

    BLINK_PLATFORM_EXPORT WebString asTraceFormat() const;
    BLINK_PLATFORM_EXPORT void assign(const WebConvertableToTraceFormat&);
    BLINK_PLATFORM_EXPORT void reset();

    WebConvertableToTraceFormat(const WebConvertableToTraceFormat& r) { assign(r); }
    WebConvertableToTraceFormat& operator=(const WebConvertableToTraceFormat& r)
    {
        assign(r);
        return *this;
    }

private:
    WebPrivatePtr<WebCore::TraceEvent::ConvertableToTraceFormat> m_private;
};

} // namespace blink

#endif
