/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef RenderSVGGradientStop_h
#define RenderSVGGradientStop_h

#include "core/rendering/RenderObject.h"

namespace WebCore {

class SVGGradientElement;
class SVGStopElement;

// This class exists mostly so we can hear about gradient stop style changes
class RenderSVGGradientStop FINAL : public RenderObject {
public:
    explicit RenderSVGGradientStop(SVGStopElement*);
    virtual ~RenderSVGGradientStop();

    virtual const char* renderName() const OVERRIDE { return "RenderSVGGradientStop"; }
    virtual bool isSVGGradientStop() const OVERRIDE { return true; }
    virtual bool isSVG() const OVERRIDE { return true; }

    virtual void layout() OVERRIDE;

    // This overrides are needed to prevent ASSERTs on <svg><stop /></svg>
    // RenderObject's default implementations ASSERT_NOT_REACHED()
    // https://bugs.webkit.org/show_bug.cgi?id=20400
    virtual LayoutRect clippedOverflowRectForPaintInvalidation(const RenderLayerModelObject*) const OVERRIDE { return LayoutRect(); }
    virtual FloatRect objectBoundingBox() const OVERRIDE { return FloatRect(); }
    virtual FloatRect strokeBoundingBox() const OVERRIDE { return FloatRect(); }
    virtual FloatRect paintInvalidationRectInLocalCoordinates() const OVERRIDE { return FloatRect(); }

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;

private:
    SVGGradientElement* gradientElement() const;
};

DEFINE_RENDER_OBJECT_TYPE_CASTS(RenderSVGGradientStop, isSVGGradientStop());

}

#endif // RenderSVGGradientStop_h
