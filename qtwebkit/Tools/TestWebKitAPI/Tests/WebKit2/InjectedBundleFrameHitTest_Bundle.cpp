/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InjectedBundleTest.h"
#include "PlatformUtilities.h"
#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFramePrivate.h>
#include <WebKit2/WKBundleHitTestResult.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

class InjectedBundleFrameHitTestTest : public InjectedBundleTest {
public:
    InjectedBundleFrameHitTestTest(const std::string& identifier)
        : InjectedBundleTest(identifier)
    {
    }

    virtual void didCreatePage(WKBundleRef, WKBundlePageRef) OVERRIDE;
    void frameLoadFinished(WKBundleFrameRef);

    WKBundleRef m_bundle;
};

static InjectedBundleTest::Register<InjectedBundleFrameHitTestTest> registrar("InjectedBundleFrameHitTestTest");

static void didFinishLoadForFrameCallback(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo)
{
    ((InjectedBundleFrameHitTestTest*)clientInfo)->frameLoadFinished(frame);
}

void InjectedBundleFrameHitTestTest::didCreatePage(WKBundleRef bundle, WKBundlePageRef page)
{
    m_bundle = bundle;

    WKBundlePageLoaderClient pageLoaderClient;
    memset(&pageLoaderClient, 0, sizeof(pageLoaderClient));
    
    pageLoaderClient.version = 1;
    pageLoaderClient.clientInfo = this;
    pageLoaderClient.didFinishLoadForFrame = didFinishLoadForFrameCallback;

    WKBundlePageSetPageLoaderClient(page, &pageLoaderClient);
}

void InjectedBundleFrameHitTestTest::frameLoadFinished(WKBundleFrameRef frame)
{
    WKBundleHitTestResultRef hitTestResult = WKBundleFrameCreateHitTestResult(frame, WKPointMake(50, 50));
    WKRetainPtr<WKStringRef> linkTitle(AdoptWK, WKBundleHitTestResultCopyLinkTitle(hitTestResult));
    WKBundlePostMessage(m_bundle, Util::toWK("InjectedBundleFrameHitTestDone").get(), linkTitle.get());
}

} // namespace TestWebKitAPI
