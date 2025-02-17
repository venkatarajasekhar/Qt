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

#ifndef ThreadableLoaderClient_h
#define ThreadableLoaderClient_h


namespace WebCore {

    class ResourceError;
    class ResourceResponse;

    class ThreadableLoaderClient {
        WTF_MAKE_NONCOPYABLE(ThreadableLoaderClient); WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual void didSendData(unsigned long long /*bytesSent*/, unsigned long long /*totalBytesToBeSent*/) { }

        virtual void didReceiveResponse(unsigned long /*identifier*/, const ResourceResponse&) { }
        virtual void didReceiveData(const char*, int /*dataLength*/) { }
        virtual void didFinishLoading(unsigned long /*identifier*/, double /*finishTime*/) { }
        virtual void didFail(const ResourceError&) { }
        virtual void didFailAccessControlCheck(const ResourceError& error) { didFail(error); }
        virtual void didFailRedirectCheck() { }

        virtual bool isDocumentThreadableLoaderClient() { return false; }

    protected:
        ThreadableLoaderClient() { }
        virtual ~ThreadableLoaderClient() { }
    };

} // namespace WebCore

#endif // ThreadableLoaderClient_h
