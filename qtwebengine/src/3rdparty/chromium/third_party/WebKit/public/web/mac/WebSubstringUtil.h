/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef WebSubstringUtil_h
#define WebSubstringUtil_h

#include "../../platform/WebCommon.h"
#include "public/web/WebFrame.h"

namespace blink {
class WebView;
struct WebPoint;
}

#if __OBJC__
@class NSAttributedString;
#else
class NSAttributedString;
#endif

namespace blink {

class WebSubstringUtil {
public:
    // Returns an autoreleased NSAttributedString that is the word under
    // the given point, or nil on error.
    // Upon return, |baselinePoint| is set to the left baseline point in
    // AppKit coordinates.
    BLINK_EXPORT static NSAttributedString* attributedWordAtPoint(WebView*,
        WebPoint,
        WebPoint& baselinePoint);

    // Returns an autoreleased NSAttributedString that is a substring of the
    // Frame at the given range, or nil on error.
    BLINK_EXPORT static NSAttributedString* attributedSubstringInRange(WebLocalFrame*,
        size_t location,
        size_t length);
};

} // namespace blink

#endif
