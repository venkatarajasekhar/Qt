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

#ifndef SVGPoint_h
#define SVGPoint_h

#include "core/svg/properties/SVGProperty.h"
#include "platform/geometry/FloatPoint.h"

namespace WebCore {

class AffineTransform;
class SVGPointTearOff;

class SVGPoint : public SVGPropertyBase {
public:
    typedef SVGPointTearOff TearOffType;

    static PassRefPtr<SVGPoint> create()
    {
        return adoptRef(new SVGPoint());
    }

    static PassRefPtr<SVGPoint> create(const FloatPoint& point)
    {
        return adoptRef(new SVGPoint(point));
    }

    PassRefPtr<SVGPoint> clone() const;
    virtual PassRefPtr<SVGPropertyBase> cloneForAnimation(const String&) const OVERRIDE;

    const FloatPoint& value() const { return m_value; }
    void setValue(const FloatPoint& value) { m_value = value; }

    float x() const { return m_value.x(); }
    float y() const { return m_value.y(); }
    void setX(float f) { m_value.setX(f); }
    void setY(float f) { m_value.setY(f); }

    FloatPoint matrixTransform(const AffineTransform&) const;

    bool operator==(const SVGPoint&) const;
    bool operator!=(const SVGPoint& other) const { return !operator==(other); }

    virtual String valueAsString() const OVERRIDE;
    void setValueAsString(const String&, ExceptionState&);

    virtual void add(PassRefPtrWillBeRawPtr<SVGPropertyBase>, SVGElement*) OVERRIDE;
    virtual void calculateAnimatedValue(SVGAnimationElement*, float percentage, unsigned repeatCount, PassRefPtr<SVGPropertyBase> from, PassRefPtr<SVGPropertyBase> to, PassRefPtr<SVGPropertyBase> toAtEndOfDurationValue, SVGElement* contextElement) OVERRIDE;
    virtual float calculateDistance(PassRefPtr<SVGPropertyBase> to, SVGElement* contextElement) OVERRIDE;

    static AnimatedPropertyType classType() { return AnimatedPoint; }

private:
    SVGPoint();
    explicit SVGPoint(const FloatPoint&);

    template<typename CharType>
    void parse(const CharType*& ptr, const CharType* end, ExceptionState&);

    FloatPoint m_value;
};

inline PassRefPtr<SVGPoint> toSVGPoint(PassRefPtr<SVGPropertyBase> passBase)
{
    RefPtr<SVGPropertyBase> base = passBase;
    ASSERT(base->type() == SVGPoint::classType());
    return static_pointer_cast<SVGPoint>(base.release());
}

} // namespace WebCore

#endif // SVGPoint_h
