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

#ifndef WebSocketImpl_h
#define WebSocketImpl_h

#include "modules/websockets/WebSocketChannelClient.h"
#include "platform/heap/Handle.h"
#include "public/platform/WebCommon.h"
#include "public/platform/WebString.h"
#include "public/web/WebSocket.h"
#include "public/web/WebSocketClient.h"
#include "wtf/OwnPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore { class WebSocketChannel; }

namespace blink {

class WebDocument;
class WebURL;

class WebSocketImpl FINAL : public WebSocket, public WebCore::WebSocketChannelClient {
public:
    WebSocketImpl(const WebDocument&, WebSocketClient*);
    virtual ~WebSocketImpl();

    bool isNull() const { return !m_private; }

    virtual BinaryType binaryType() const OVERRIDE;
    virtual bool setBinaryType(BinaryType) OVERRIDE;
    virtual void connect(const WebURL&, const WebString& protocol) OVERRIDE;
    virtual WebString subprotocol() OVERRIDE;
    virtual WebString extensions() OVERRIDE;
    virtual bool sendText(const WebString&) OVERRIDE;
    virtual bool sendArrayBuffer(const WebArrayBuffer&) OVERRIDE;
    virtual unsigned long bufferedAmount() const OVERRIDE;
    virtual void close(int code, const WebString& reason) OVERRIDE;
    virtual void fail(const WebString& reason) OVERRIDE;
    virtual void disconnect() OVERRIDE;

    // WebSocketChannelClient
    virtual void didConnect(const String& subprotocol, const String& extensions) OVERRIDE;
    virtual void didReceiveMessage(const String& message) OVERRIDE;
    virtual void didReceiveBinaryData(PassOwnPtr<Vector<char> > binaryData) OVERRIDE;
    virtual void didReceiveMessageError() OVERRIDE;
    virtual void didConsumeBufferedAmount(unsigned long consumed) OVERRIDE;
    virtual void didStartClosingHandshake() OVERRIDE;
    virtual void didClose(ClosingHandshakeCompletionStatus, unsigned short code, const String& reason) OVERRIDE;

private:
    RefPtrWillBePersistent<WebCore::WebSocketChannel> m_private;
    WebSocketClient* m_client;
    BinaryType m_binaryType;
    WebString m_subprotocol;
    WebString m_extensions;
    bool m_isClosingOrClosed;
    // m_bufferedAmount includes m_bufferedAmountAfterClose.
    unsigned long m_bufferedAmount;
    unsigned long m_bufferedAmountAfterClose;
};

} // namespace blink

#endif // WebWebSocketChannelImpl_h
