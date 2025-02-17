/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#ifndef WebHeap_h
#define WebHeap_h

#include "public/platform/WebCommon.h"

namespace blink {

class WebHeap {
public:
    // Attach thread to the garbage collector managed heap. Thread can
    // allocate or use garbage collector managed objects only while it is
    // attached.
    BLINK_EXPORT static void attachThread();

    // Detach thread from the garbage collector managed heap. Thread can
    // no longer allocate or use garbage collector managed objects.
    BLINK_EXPORT static void detachThread();

    // While this object is active on the stack current thread is marked as
    // being at safepoint. It can't manipulate garbage collector managed objects
    // until it leaves safepoint, but it can block indefinitely.
    // When thread is not at safe-point it must not block indefinitely because
    // garbage collector might want to stop it.
    class SafePointScope {
    public:
        BLINK_EXPORT SafePointScope();
        BLINK_EXPORT ~SafePointScope();
    };
};

} // namespace blink

#endif
