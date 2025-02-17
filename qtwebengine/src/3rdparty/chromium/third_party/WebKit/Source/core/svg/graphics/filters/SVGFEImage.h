/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef SVGFEImage_h
#define SVGFEImage_h

#include "core/dom/TreeScope.h"
#include "core/svg/SVGPreserveAspectRatio.h"
#include "platform/graphics/filters/FilterEffect.h"

namespace WebCore {

class Image;
class RenderObject;

class FEImage FINAL : public FilterEffect {
public:
    static PassRefPtr<FEImage> createWithImage(Filter*, PassRefPtr<Image>, PassRefPtr<SVGPreserveAspectRatio>);
    static PassRefPtr<FEImage> createWithIRIReference(Filter*, TreeScope&, const String&, PassRefPtr<SVGPreserveAspectRatio>);

    virtual FloatRect determineAbsolutePaintRect(const FloatRect& requestedRect) OVERRIDE;

    virtual FilterEffectType filterEffectType() const OVERRIDE { return FilterEffectTypeImage; }

    virtual TextStream& externalRepresentation(TextStream&, int indention) const OVERRIDE;
    virtual PassRefPtr<SkImageFilter> createImageFilter(SkiaImageFilterBuilder*) OVERRIDE;

private:
    virtual ~FEImage() { }
    FEImage(Filter*, PassRefPtr<Image>, PassRefPtr<SVGPreserveAspectRatio>);
    FEImage(Filter*, TreeScope&, const String&, PassRefPtr<SVGPreserveAspectRatio>);
    RenderObject* referencedRenderer() const;

    virtual void applySoftware() OVERRIDE;
    PassRefPtr<SkImageFilter> createImageFilterForRenderer(RenderObject* rendererer, SkiaImageFilterBuilder*);

    RefPtr<Image> m_image;

    // m_treeScope will never be a dangling reference. See https://bugs.webkit.org/show_bug.cgi?id=99243
    TreeScope* m_treeScope;
    String m_href;
    PassRefPtr<SVGPreserveAspectRatio> m_preserveAspectRatio;
};

} // namespace WebCore

#endif // SVGFEImage_h
