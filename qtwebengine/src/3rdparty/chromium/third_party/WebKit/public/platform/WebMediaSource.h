/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebMediaSource_h
#define WebMediaSource_h

#include "WebCommon.h"
#include "WebTimeRange.h"
#include "WebURL.h"

namespace blink {

class WebSourceBuffer;

class WebMediaSource {
public:
    enum AddStatus {
        AddStatusOk,
        AddStatusNotSupported,
        AddStatusReachedIdLimit
    };

    enum EndOfStreamStatus {
        EndOfStreamStatusNoError,
        EndOfStreamStatusNetworkError,
        EndOfStreamStatusDecodeError,
    };

    enum FrameProcessorChoice {
        UseLegacyFrameProcessor,
        UseNewFrameProcessor
    };

    virtual ~WebMediaSource() { }
    // FIXME: Remove addSourceBuffer() that has FrameProcessorChoice, and remove
    // the default implementations for addSourceBuffer() once Chromium
    // implementation of the addSourceBuffer() with no FrameProcessorChoice has
    // landed. See http://crbug.com/249422.
    virtual AddStatus addSourceBuffer(const WebString& type, const WebVector<WebString>& codecs, const FrameProcessorChoice, WebSourceBuffer** webSourceBuffer)
    {
        // This default implementation should never be called for real.
        BLINK_ASSERT_NOT_REACHED();
        return AddStatusReachedIdLimit;
    }
    virtual AddStatus addSourceBuffer(const WebString& type, const WebVector<WebString>& codecs, WebSourceBuffer** webSourceBuffer)
    {
        return addSourceBuffer(type, codecs, WebMediaSource::UseNewFrameProcessor, webSourceBuffer);
    }

    virtual double duration() = 0;
    virtual void setDuration(double) = 0;
    virtual void markEndOfStream(EndOfStreamStatus) = 0;
    virtual void unmarkEndOfStream() = 0;
};

} // namespace blink

#endif
