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

#ifndef StorageQuotaClientImpl_h
#define StorageQuotaClientImpl_h

#include "modules/quota/StorageQuotaClient.h"
#include "wtf/Forward.h"

namespace blink {

class StorageQuotaClientImpl : public NoBaseWillBeGarbageCollectedFinalized<StorageQuotaClientImpl>, public WebCore::StorageQuotaClient {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(StorageQuotaClientImpl);
public:
    static PassOwnPtrWillBeRawPtr<StorageQuotaClientImpl> create();

    virtual ~StorageQuotaClientImpl();

    virtual void requestQuota(WebCore::ExecutionContext*, WebStorageQuotaType, unsigned long long newQuotaInBytes, PassOwnPtr<WebCore::StorageQuotaCallback>, PassOwnPtr<WebCore::StorageErrorCallback>) OVERRIDE;
    virtual WebCore::ScriptPromise requestPersistentQuota(WebCore::ScriptState*, unsigned long long newQuotaInBytes) OVERRIDE;

    virtual void trace(WebCore::Visitor* visitor) OVERRIDE { WebCore::StorageQuotaClient::trace(visitor); }

private:
    StorageQuotaClientImpl();
};

} // namespace blink

#endif // StorageQuotaClientImpl_h
