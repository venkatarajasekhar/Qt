/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "JSHTMLEmbedElement.h"

#include "HTMLEmbedElement.h"
#include "JSPluginElementFunctions.h"

namespace WebCore {

using namespace JSC;

bool JSHTMLEmbedElement::getOwnPropertySlotDelegate(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return pluginElementCustomGetOwnPropertySlot<JSHTMLEmbedElement, Base>(exec, propertyName, slot, this);
}

bool JSHTMLEmbedElement::getOwnPropertyDescriptorDelegate(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return pluginElementCustomGetOwnPropertyDescriptor<JSHTMLEmbedElement, Base>(exec, propertyName, descriptor, this);
}

bool JSHTMLEmbedElement::putDelegate(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    return runtimeObjectCustomPut(exec, propertyName, value, this, slot);
}

CallType JSHTMLEmbedElement::getCallData(JSCell* cell, CallData& callData)
{
    return runtimeObjectGetCallData(jsCast<JSHTMLEmbedElement*>(cell), callData);
}

} // namespace WebCore
