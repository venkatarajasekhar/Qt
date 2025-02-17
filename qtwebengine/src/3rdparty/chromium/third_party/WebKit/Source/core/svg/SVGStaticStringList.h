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

#ifndef SVGStaticStringList_h
#define SVGStaticStringList_h

#include "core/svg/SVGStringListTearOff.h"
#include "core/svg/properties/SVGAnimatedProperty.h"

namespace WebCore {

class SVGElement;

// SVGStringList property implementations for SVGTests properties.
// Inherits SVGAnimatedPropertyBase to enable XML attribute synchronization, but this is never animated.
class SVGStaticStringList FINAL : public SVGAnimatedPropertyBase {
public:
    static PassRefPtr<SVGStaticStringList> create(SVGElement* contextElement, const QualifiedName& attributeName)
    {
        return adoptRef(new SVGStaticStringList(contextElement, attributeName));
    }

    virtual ~SVGStaticStringList();

    // SVGAnimatedPropertyBase:
    virtual SVGPropertyBase* currentValueBase() OVERRIDE;
    virtual bool isAnimating() const OVERRIDE;
    virtual PassRefPtr<SVGPropertyBase> createAnimatedValue() OVERRIDE;
    virtual void setAnimatedValue(PassRefPtr<SVGPropertyBase>) OVERRIDE;
    virtual void animationEnded() OVERRIDE;
    virtual bool needsSynchronizeAttribute() OVERRIDE;

    void setBaseValueAsString(const String& value, SVGParsingError& parseError);

    SVGStringList* value() { return m_value.get(); }
    SVGStringListTearOff* tearOff();

private:
    SVGStaticStringList(SVGElement*, const QualifiedName&);

    RefPtr<SVGStringList> m_value;
    RefPtr<SVGStringListTearOff> m_tearOff;
};

}

#endif
