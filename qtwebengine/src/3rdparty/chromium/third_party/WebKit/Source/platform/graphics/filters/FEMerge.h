/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#ifndef FEMerge_h
#define FEMerge_h

#include "platform/graphics/filters/Filter.h"
#include "platform/graphics/filters/FilterEffect.h"
#include "wtf/Vector.h"

namespace WebCore {

class PLATFORM_EXPORT FEMerge : public FilterEffect {
public:
    static PassRefPtr<FEMerge> create(Filter*);

    virtual PassRefPtr<SkImageFilter> createImageFilter(SkiaImageFilterBuilder*) OVERRIDE;

    virtual TextStream& externalRepresentation(TextStream&, int indention) const OVERRIDE;

private:
    FEMerge(Filter*);

    virtual void applySoftware() OVERRIDE;
};

} // namespace WebCore

#endif // FEMerge_h
