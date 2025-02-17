/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#ifndef StyleGeneratedImage_h
#define StyleGeneratedImage_h

#include "core/rendering/style/StyleImage.h"

namespace WebCore {

class CSSValue;
class CSSImageGeneratorValue;

class StyleGeneratedImage FINAL : public StyleImage {
public:
    static PassRefPtr<StyleGeneratedImage> create(CSSImageGeneratorValue* value)
    {
        return adoptRef(new StyleGeneratedImage(value));
    }

    virtual WrappedImagePtr data() const OVERRIDE { return m_imageGeneratorValue.get(); }

    virtual PassRefPtrWillBeRawPtr<CSSValue> cssValue() const OVERRIDE;

    virtual LayoutSize imageSize(const RenderObject*, float multiplier) const OVERRIDE;
    virtual bool imageHasRelativeWidth() const OVERRIDE { return !m_fixedSize; }
    virtual bool imageHasRelativeHeight() const OVERRIDE { return !m_fixedSize; }
    virtual void computeIntrinsicDimensions(const RenderObject*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) OVERRIDE;
    virtual bool usesImageContainerSize() const OVERRIDE { return !m_fixedSize; }
    virtual void setContainerSizeForRenderer(const RenderObject*, const IntSize& containerSize, float) OVERRIDE { m_containerSize = containerSize; }
    virtual void addClient(RenderObject*) OVERRIDE;
    virtual void removeClient(RenderObject*) OVERRIDE;
    virtual PassRefPtr<Image> image(RenderObject*, const IntSize&) const OVERRIDE;
    virtual bool knownToBeOpaque(const RenderObject*) const OVERRIDE;

private:
    StyleGeneratedImage(PassRefPtrWillBeRawPtr<CSSImageGeneratorValue>);

    // FIXME: oilpan: change to member once StyleImage is moved to the oilpan heap
    RefPtrWillBePersistent<CSSImageGeneratorValue> m_imageGeneratorValue;
    IntSize m_containerSize;
    bool m_fixedSize;
};

}
#endif
