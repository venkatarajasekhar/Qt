// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LegacyStyleInterpolation_h
#define LegacyStyleInterpolation_h

#include "core/animation/interpolation/StyleInterpolation.h"
#include "core/css/resolver/AnimatedStyleBuilder.h"

namespace WebCore {

class LegacyStyleInterpolation : public StyleInterpolation {
public:
    static PassRefPtrWillBeRawPtr<LegacyStyleInterpolation> create(PassRefPtrWillBeRawPtr<AnimatableValue> start, PassRefPtrWillBeRawPtr<AnimatableValue> end, CSSPropertyID id)
    {
        return adoptRefWillBeNoop(new LegacyStyleInterpolation(InterpolableAnimatableValue::create(start), InterpolableAnimatableValue::create(end), id));
    }

    virtual void apply(StyleResolverState& state) const OVERRIDE
    {
        AnimatedStyleBuilder::applyProperty(m_id, state, currentValue().get());
    }

    virtual bool isLegacyStyleInterpolation() const OVERRIDE FINAL { return true; }
    PassRefPtrWillBeRawPtr<AnimatableValue> currentValue() const
    {
        InterpolableAnimatableValue* value = static_cast<InterpolableAnimatableValue*>(m_cachedValue.get());
        return value->value();
    }

    virtual void trace(Visitor* visitor) OVERRIDE
    {
        StyleInterpolation::trace(visitor);
    }

private:
    LegacyStyleInterpolation(PassOwnPtrWillBeRawPtr<InterpolableValue> start, PassOwnPtrWillBeRawPtr<InterpolableValue> end, CSSPropertyID id)
        : StyleInterpolation(start, end, id)
    {
    }
};

DEFINE_TYPE_CASTS(LegacyStyleInterpolation, Interpolation, value, value->isLegacyStyleInterpolation(), value.isLegacyStyleInterpolation());

}

#endif
