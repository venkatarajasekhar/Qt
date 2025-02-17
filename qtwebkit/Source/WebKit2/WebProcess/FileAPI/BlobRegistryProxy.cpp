/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "BlobRegistryProxy.h"

#if ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)

#include "BlobRegistrationData.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include <WebCore/BlobData.h>

using namespace WebCore;

namespace WebKit {

void BlobRegistryProxy::registerBlobURL(const KURL& url, PassOwnPtr<BlobData> blobData)
{
    ASSERT(WebProcess::shared().usesNetworkProcess());

    WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::RegisterBlobURL(url, BlobRegistrationData(blobData)), 0);
}

void BlobRegistryProxy::registerBlobURL(const KURL& url, const KURL& srcURL)
{
    ASSERT(WebProcess::shared().usesNetworkProcess());

    WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::RegisterBlobURLFromURL(url, srcURL), 0);
}

void BlobRegistryProxy::unregisterBlobURL(const KURL& url)
{
    ASSERT(WebProcess::shared().usesNetworkProcess());

    WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::UnregisterBlobURL(url), 0);
}

}

#endif // ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)
