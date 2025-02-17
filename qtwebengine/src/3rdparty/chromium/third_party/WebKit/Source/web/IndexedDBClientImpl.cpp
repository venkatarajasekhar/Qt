/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "web/IndexedDBClientImpl.h"

#include "bindings/v8/WorkerScriptController.h"
#include "core/dom/Document.h"
#include "core/workers/WorkerGlobalScope.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/web/WebKit.h"
#include "public/web/WebPermissionClient.h"
#include "public/web/WebSecurityOrigin.h"
#include "web/WebLocalFrameImpl.h"
#include "web/WorkerPermissionClient.h"


using namespace WebCore;

namespace blink {

IndexedDBClient* IndexedDBClientImpl::create()
{
    return new IndexedDBClientImpl();
}

bool IndexedDBClientImpl::allowIndexedDB(ExecutionContext* context, const String& name)
{
    ASSERT_WITH_SECURITY_IMPLICATION(context->isDocument() || context->isWorkerGlobalScope());

    if (context->isDocument()) {
        WebSecurityOrigin origin(context->securityOrigin());
        Document* document = toDocument(context);
        WebLocalFrameImpl* webFrame = WebLocalFrameImpl::fromFrame(document->frame());
        // FIXME: webFrame->permissionClient() returns 0 in test_shell and content_shell http://crbug.com/137269
        return !webFrame->permissionClient() || webFrame->permissionClient()->allowIndexedDB(name, origin);
    }

    WorkerGlobalScope& workerGlobalScope = *toWorkerGlobalScope(context);
    return WorkerPermissionClient::from(workerGlobalScope)->allowIndexedDB(name);
}

} // namespace blink
