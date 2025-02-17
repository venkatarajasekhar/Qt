/*
 * Copyright (C) 2011, 2012 Google Inc.  All rights reserved.
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

#ifndef WebSocketClient_h
#define WebSocketClient_h

#include "../platform/WebCommon.h"

namespace blink {

class WebArrayBuffer;
class WebString;

class WebSocketClient {
public:
    enum ClosingHandshakeCompletionStatus {
        ClosingHandshakeIncomplete,
        ClosingHandshakeComplete
    };

    virtual ~WebSocketClient() { }
    virtual void didConnect(const WebString& subprotocol, const WebString& extensions) { }
    virtual void didReceiveMessage(const WebString& message) { }
    virtual void didReceiveArrayBuffer(const WebArrayBuffer& arrayBuffer) { }
    virtual void didReceiveMessageError() { }
    virtual void didConsumeBufferedAmount(unsigned long consumed) { }
    virtual void didStartClosingHandshake() { }
    virtual void didClose(ClosingHandshakeCompletionStatus, unsigned short code, const WebString& reason) { }

    // FIXME: Deperecate these methods.
    virtual void didConnect() { }
    virtual void didUpdateBufferedAmount(unsigned long bufferedAmount) { }
    virtual void didClose(unsigned long unhandledBufferedAmount, ClosingHandshakeCompletionStatus, unsigned short code, const WebString& reason) { }
};

} // namespace blink

#endif // WebSocketClient_h
