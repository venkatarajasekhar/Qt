/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebDataSourceImpl_h
#define WebDataSourceImpl_h

#include "core/loader/DocumentLoader.h"
#include "platform/exported/WrappedResourceRequest.h"
#include "platform/exported/WrappedResourceResponse.h"
#include "platform/weborigin/KURL.h"
#include "public/web/WebDataSource.h"
#include "web/WebPluginLoadObserver.h"
#include "wtf/OwnPtr.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/Vector.h"

namespace blink {

class WebPluginLoadObserver;

class WebDataSourceImpl FINAL : public WebCore::DocumentLoader, public WebDataSource {
public:
    static PassRefPtr<WebDataSourceImpl> create(WebCore::LocalFrame*, const WebCore::ResourceRequest&, const WebCore::SubstituteData&);

    static WebDataSourceImpl* fromDocumentLoader(WebCore::DocumentLoader* loader)
    {
        return static_cast<WebDataSourceImpl*>(loader);
    }

    // WebDataSource methods:
    virtual const WebURLRequest& originalRequest() const OVERRIDE;
    virtual const WebURLRequest& request() const OVERRIDE;
    virtual const WebURLResponse& response() const OVERRIDE;
    virtual bool hasUnreachableURL() const OVERRIDE;
    virtual WebURL unreachableURL() const OVERRIDE;
    virtual void appendRedirect(const WebURL&) OVERRIDE;
    virtual void redirectChain(WebVector<WebURL>&) const OVERRIDE;
    virtual bool isClientRedirect() const OVERRIDE;
    virtual bool replacesCurrentHistoryItem() const OVERRIDE;
    virtual WebNavigationType navigationType() const OVERRIDE;
    virtual double triggeringEventTime() const OVERRIDE;
    virtual ExtraData* extraData() const OVERRIDE;
    virtual void setExtraData(ExtraData*) OVERRIDE;
    virtual void setNavigationStartTime(double) OVERRIDE;

    static WebNavigationType toWebNavigationType(WebCore::NavigationType type);

    PassOwnPtr<WebPluginLoadObserver> releasePluginLoadObserver() { return m_pluginLoadObserver.release(); }
    static void setNextPluginLoadObserver(PassOwnPtr<WebPluginLoadObserver>);

private:
    WebDataSourceImpl(WebCore::LocalFrame*, const WebCore::ResourceRequest&, const WebCore::SubstituteData&);
    virtual ~WebDataSourceImpl();

    // Mutable because the const getters will magically sync these to the
    // latest version from WebKit.
    mutable WrappedResourceRequest m_originalRequestWrapper;
    mutable WrappedResourceRequest m_requestWrapper;
    mutable WrappedResourceResponse m_responseWrapper;

    OwnPtr<ExtraData> m_extraData;
    OwnPtr<WebPluginLoadObserver> m_pluginLoadObserver;
};

} // namespace blink

#endif  // WebDataSourceImpl_h
