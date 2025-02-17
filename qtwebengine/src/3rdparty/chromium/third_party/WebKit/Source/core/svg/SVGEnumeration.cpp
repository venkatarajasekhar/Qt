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

#include "config.h"

#include "core/svg/SVGEnumeration.h"

#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/dom/ExceptionCode.h"
#include "core/svg/SVGAnimationElement.h"

namespace WebCore {

inline PassRefPtr<SVGEnumerationBase> toSVGEnumerationBase(PassRefPtr<SVGPropertyBase> passBase)
{
    RefPtr<SVGPropertyBase> base = passBase;
    ASSERT(base->type() == SVGEnumerationBase::classType());
    return static_pointer_cast<SVGEnumerationBase>(base.release());
}

SVGEnumerationBase::~SVGEnumerationBase()
{
}

PassRefPtr<SVGPropertyBase> SVGEnumerationBase::cloneForAnimation(const String& value) const
{
    RefPtr<SVGEnumerationBase> svgEnumeration = clone();
    svgEnumeration->setValueAsString(value, IGNORE_EXCEPTION);
    return svgEnumeration.release();
}

String SVGEnumerationBase::valueAsString() const
{
    StringEntries::const_iterator it = m_entries.begin();
    StringEntries::const_iterator itEnd = m_entries.end();
    for (; it != itEnd; ++it) {
        if (m_value == it->first)
            return it->second;
    }

    ASSERT(m_value < maxEnumValue());
    return emptyString();
}

void SVGEnumerationBase::setValue(unsigned short value, ExceptionState& exceptionState)
{
    if (!value) {
        exceptionState.throwTypeError("The enumeration value provided is 0, which is not settable.");
        return;
    }

    if (value > maxEnumValue()) {
        exceptionState.throwTypeError("The enumeration value provided (" + String::number(value) + ") is larger than the largest allowed value (" + String::number(maxEnumValue()) + ").");
        return;
    }

    m_value = value;
    notifyChange();
}

void SVGEnumerationBase::setValueAsString(const String& string, ExceptionState& exceptionState)
{
    StringEntries::const_iterator it = m_entries.begin();
    StringEntries::const_iterator itEnd = m_entries.end();
    for (; it != itEnd; ++it) {
        if (string == it->second) {
            // 0 corresponds to _UNKNOWN enumeration values, and should not be settable.
            ASSERT(it->first);
            m_value = it->first;
            notifyChange();
            return;
        }
    }

    exceptionState.throwDOMException(SyntaxError, "The value provided ('" + string + "') is invalid.");
    notifyChange();
}

void SVGEnumerationBase::add(PassRefPtrWillBeRawPtr<SVGPropertyBase>, SVGElement*)
{
    ASSERT_NOT_REACHED();
}

void SVGEnumerationBase::calculateAnimatedValue(SVGAnimationElement* animationElement, float percentage, unsigned repeatCount, PassRefPtr<SVGPropertyBase> from, PassRefPtr<SVGPropertyBase> to, PassRefPtr<SVGPropertyBase>, SVGElement*)
{
    ASSERT(animationElement);
    unsigned short fromEnumeration = animationElement->animationMode() == ToAnimation ? m_value : toSVGEnumerationBase(from)->value();
    unsigned short toEnumeration = toSVGEnumerationBase(to)->value();

    animationElement->animateDiscreteType<unsigned short>(percentage, fromEnumeration, toEnumeration, m_value);
}

float SVGEnumerationBase::calculateDistance(PassRefPtr<SVGPropertyBase>, SVGElement*)
{
    // No paced animations for boolean.
    return -1;
}

}
