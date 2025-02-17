/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGPathByteStreamBuilder_h
#define SVGPathByteStreamBuilder_h

#include "core/svg/SVGPathByteStream.h"
#include "core/svg/SVGPathConsumer.h"
#include "platform/geometry/FloatPoint.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class SVGPathByteStreamBuilder FINAL : public SVGPathConsumer {
public:
    SVGPathByteStreamBuilder();

    void setCurrentByteStream(SVGPathByteStream* byteStream) { m_byteStream = byteStream; }

private:
    virtual void incrementPathSegmentCount() OVERRIDE { }
    virtual bool continueConsuming() OVERRIDE { return true; }
    virtual void cleanup() OVERRIDE { m_byteStream = 0; }

    // Used in UnalteredParsing/NormalizedParsing modes.
    virtual void moveTo(const FloatPoint&, bool closed, PathCoordinateMode) OVERRIDE;
    virtual void lineTo(const FloatPoint&, PathCoordinateMode) OVERRIDE;
    virtual void curveToCubic(const FloatPoint&, const FloatPoint&, const FloatPoint&, PathCoordinateMode) OVERRIDE;
    virtual void closePath() OVERRIDE;

    // Only used in UnalteredParsing mode.
    virtual void lineToHorizontal(float, PathCoordinateMode) OVERRIDE;
    virtual void lineToVertical(float, PathCoordinateMode) OVERRIDE;
    virtual void curveToCubicSmooth(const FloatPoint&, const FloatPoint&, PathCoordinateMode) OVERRIDE;
    virtual void curveToQuadratic(const FloatPoint&, const FloatPoint&, PathCoordinateMode) OVERRIDE;
    virtual void curveToQuadraticSmooth(const FloatPoint&, PathCoordinateMode) OVERRIDE;
    virtual void arcTo(float, float, float, bool largeArcFlag, bool sweepFlag, const FloatPoint&, PathCoordinateMode) OVERRIDE;

    SVGPathByteStream* m_byteStream;
};

} // namespace WebCore

#endif // SVGPathByteStreamBuilder_h
