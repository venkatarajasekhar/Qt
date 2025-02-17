/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DOMWindowLifecycleNotifier_h
#define DOMWindowLifecycleNotifier_h

#include "core/frame/DOMWindowLifecycleObserver.h"
#include "platform/LifecycleNotifier.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/TemporaryChange.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class LocalDOMWindow;

class DOMWindowLifecycleNotifier FINAL : public LifecycleNotifier<LocalDOMWindow> {
public:
    static PassOwnPtr<DOMWindowLifecycleNotifier> create(LocalDOMWindow*);

    void notifyAddEventListener(LocalDOMWindow*, const AtomicString& eventType);
    void notifyRemoveEventListener(LocalDOMWindow*, const AtomicString& eventType);
    void notifyRemoveAllEventListeners(LocalDOMWindow*);

    virtual void addObserver(Observer*) OVERRIDE;
    virtual void removeObserver(Observer*) OVERRIDE;

private:
    explicit DOMWindowLifecycleNotifier(LocalDOMWindow*);

    typedef HashSet<DOMWindowLifecycleObserver*> DOMWindowObserverSet;
    DOMWindowObserverSet m_windowObservers;
};

} // namespace WebCore

#endif // DOMWindowLifecycleNotifier_h
