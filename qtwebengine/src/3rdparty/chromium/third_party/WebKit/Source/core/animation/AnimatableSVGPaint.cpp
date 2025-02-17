/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
#include "core/animation/AnimatableSVGPaint.h"

namespace WebCore {

bool AnimatableSVGPaint::usesDefaultInterpolationWith(const AnimatableValue* value) const
{
    const AnimatableSVGPaint* svgPaint = toAnimatableSVGPaint(value);
    return (paintType() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR || svgPaint->paintType() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR)
        && (visitedLinkPaintType() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR || svgPaint->visitedLinkPaintType() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR);
}

PassRefPtrWillBeRawPtr<AnimatableValue> AnimatableSVGPaint::interpolateTo(const AnimatableValue* value, double fraction) const
{
    if (usesDefaultInterpolationWith(value))
        return defaultInterpolateTo(this, value, fraction);

    const AnimatableSVGPaint* svgPaint = toAnimatableSVGPaint(value);
    RefPtrWillBeRawPtr<AnimatableColor> color = toAnimatableColor(AnimatableValue::interpolate(m_color.get(), svgPaint->m_color.get(), fraction).get());
    if (fraction < 0.5)
        return create(paintType(), visitedLinkPaintType(), color, uri(), visitedLinkURI());
    return create(svgPaint->paintType(), svgPaint->visitedLinkPaintType(), color, svgPaint->uri(), svgPaint->visitedLinkURI());
}

bool AnimatableSVGPaint::equalTo(const AnimatableValue* value) const
{
    const AnimatableSVGPaint* svgPaint = toAnimatableSVGPaint(value);
    return paintType() == svgPaint->paintType()
        && visitedLinkPaintType() == svgPaint->visitedLinkPaintType()
        && color() == svgPaint->color()
        && uri() == svgPaint->uri()
        && visitedLinkURI() == svgPaint->visitedLinkURI();
}

}
