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

#ifndef WebTextAreaElement_h
#define WebTextAreaElement_h

#include "WebFormControlElement.h"

#if BLINK_IMPLEMENTATION
namespace WebCore { class HTMLTextAreaElement; }
#endif

namespace blink {

// Provides access to some properties of a DOM textarea element node.
class WebTextAreaElement : public WebFormControlElement {
public:
    WebTextAreaElement() : WebFormControlElement() { }
    WebTextAreaElement(const WebTextAreaElement& element) : WebFormControlElement(element) { }

    WebTextAreaElement& operator=(const WebTextAreaElement& element)
    {
        WebFormControlElement::assign(element);
        return *this;
    }
    void assign(const WebTextAreaElement& element) { WebFormControlElement::assign(element); }

#if BLINK_IMPLEMENTATION
    WebTextAreaElement(const PassRefPtrWillBeRawPtr<WebCore::HTMLTextAreaElement>&);
    WebTextAreaElement& operator=(const PassRefPtrWillBeRawPtr<WebCore::HTMLTextAreaElement>&);
    operator PassRefPtrWillBeRawPtr<WebCore::HTMLTextAreaElement>() const;
#endif
};

} // namespace blink

#endif
