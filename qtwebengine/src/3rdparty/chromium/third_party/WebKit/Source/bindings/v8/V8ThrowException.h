/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef V8ThrowException_h
#define V8ThrowException_h

#include "wtf/text/WTFString.h"
#include <v8.h>

namespace WebCore {

enum V8ErrorType {
    v8RangeError,
    v8ReferenceError,
    v8SyntaxError,
    v8TypeError,
    v8GeneralError
};

class V8ThrowException {
public:

    static v8::Handle<v8::Value> createDOMException(int ec, const String& message, const v8::Handle<v8::Object>& creationContext, v8::Isolate* isolate)
    {
        return createDOMException(ec, message, String(), creationContext, isolate);
    }
    static v8::Handle<v8::Value> createDOMException(int, const String& sanitizedMessage, const String& unsanitizedMessage, const v8::Handle<v8::Object>& creationContext, v8::Isolate*);

    static v8::Handle<v8::Value> throwDOMException(int ec, const String& message, const v8::Handle<v8::Object>& creationContext, v8::Isolate* isolate)
    {
        return throwDOMException(ec, message, String(), creationContext, isolate);
    }
    static v8::Handle<v8::Value> throwDOMException(int, const String& sanitizedMessage, const String& unsanitizedMessage, const v8::Handle<v8::Object>& creationContext, v8::Isolate*);

    static v8::Handle<v8::Value> createError(V8ErrorType, const String&, v8::Isolate*);
    static v8::Handle<v8::Value> throwError(V8ErrorType, const String&, v8::Isolate*);
    static v8::Handle<v8::Value> throwError(v8::Handle<v8::Value>, v8::Isolate*);

    static v8::Handle<v8::Value> createTypeError(const String&, v8::Isolate*);
    static v8::Handle<v8::Value> throwTypeError(const String&, v8::Isolate*);
};

} // namespace WebCore

#endif // V8ThrowException_h
