/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebKitTransitionEvent.h"

#include "EventNames.h"

namespace WebCore {

WebKitTransitionEventInit::WebKitTransitionEventInit()
    : elapsedTime(0)
{
}

WebKitTransitionEvent::WebKitTransitionEvent()
    : m_elapsedTime(0)
{
}

WebKitTransitionEvent::WebKitTransitionEvent(const AtomicString& type, const String& propertyName, double elapsedTime, const String& pseudoElement)
    : Event(type, true, true)
    , m_propertyName(propertyName)
    , m_elapsedTime(elapsedTime)
    , m_pseudoElement(pseudoElement)
{
}

WebKitTransitionEvent::WebKitTransitionEvent(const AtomicString& type, const WebKitTransitionEventInit& initializer)
    : Event(type, initializer)
    , m_propertyName(initializer.propertyName)
    , m_elapsedTime(initializer.elapsedTime)
    , m_pseudoElement(initializer.pseudoElement)
{
}

WebKitTransitionEvent::~WebKitTransitionEvent()
{
}

const String& WebKitTransitionEvent::propertyName() const
{
    return m_propertyName;
}

double WebKitTransitionEvent::elapsedTime() const
{
    return m_elapsedTime;
}

const String& WebKitTransitionEvent::pseudoElement() const
{
    return m_pseudoElement;
}

const AtomicString& WebKitTransitionEvent::interfaceName() const
{
    return eventNames().interfaceForWebKitTransitionEvent;
}

} // namespace WebCore
