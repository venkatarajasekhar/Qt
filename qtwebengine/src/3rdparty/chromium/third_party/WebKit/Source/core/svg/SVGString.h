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

#ifndef SVGString_h
#define SVGString_h

#include "core/svg/properties/SVGProperty.h"

namespace WebCore {

class SVGString : public SVGPropertyBase {
public:
    // SVGString does not have a tear-off type.
    typedef void TearOffType;
    typedef String PrimitiveType;

    static PassRefPtr<SVGString> create()
    {
        return adoptRef(new SVGString());
    }

    static PassRefPtr<SVGString> create(const String& value)
    {
        return adoptRef(new SVGString(value));
    }

    PassRefPtr<SVGString> clone() const { return create(m_value); }
    virtual PassRefPtr<SVGPropertyBase> cloneForAnimation(const String& value) const OVERRIDE
    {
        return create(value);
    }

    virtual String valueAsString() const OVERRIDE { return m_value; }
    void setValueAsString(const String& value, ExceptionState&) { m_value = value; }

    virtual void add(PassRefPtrWillBeRawPtr<SVGPropertyBase>, SVGElement*) OVERRIDE;
    virtual void calculateAnimatedValue(SVGAnimationElement*, float percentage, unsigned repeatCount, PassRefPtr<SVGPropertyBase> from, PassRefPtr<SVGPropertyBase> to, PassRefPtr<SVGPropertyBase> toAtEndOfDurationValue, SVGElement*) OVERRIDE;
    virtual float calculateDistance(PassRefPtr<SVGPropertyBase> to, SVGElement*) OVERRIDE;

    const String& value() const { return m_value; }
    void setValue(const String& value) { m_value = value; }

    static AnimatedPropertyType classType() { return AnimatedString; }

private:
    SVGString()
        : SVGPropertyBase(classType())
    {
    }

    explicit SVGString(const String& value)
        : SVGPropertyBase(classType())
        , m_value(value)
    {
    }

    String m_value;
};

inline PassRefPtr<SVGString> toSVGString(PassRefPtr<SVGPropertyBase> passBase)
{
    RefPtr<SVGPropertyBase> base = passBase;
    ASSERT(base->type() == SVGString::classType());
    return static_pointer_cast<SVGString>(base.release());
}

} // namespace WebCore

#endif // SVGString_h
