/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#include "config.h"
#include "core/svg/SVGTransform.h"

#include "platform/FloatConversion.h"
#include "platform/geometry/FloatSize.h"
#include "wtf/MathExtras.h"
#include "wtf/text/StringBuilder.h"

namespace WebCore {

SVGTransform::SVGTransform()
    : SVGPropertyBase(classType())
    , m_transformType(SVG_TRANSFORM_UNKNOWN)
    , m_angle(0)
{
}

SVGTransform::SVGTransform(SVGTransformType transformType, ConstructionMode mode)
    : SVGPropertyBase(classType())
    , m_transformType(transformType)
    , m_angle(0)
{
    if (mode == ConstructZeroTransform)
        m_matrix = AffineTransform(0, 0, 0, 0, 0, 0);
}

SVGTransform::SVGTransform(const AffineTransform& matrix)
    : SVGPropertyBase(classType())
    , m_transformType(SVG_TRANSFORM_MATRIX)
    , m_angle(0)
    , m_matrix(matrix)
{
}

SVGTransform::SVGTransform(SVGTransformType transformType, float angle, const FloatPoint& center, const AffineTransform& matrix)
    : SVGPropertyBase(classType())
    , m_transformType(transformType)
    , m_angle(angle)
    , m_center(center)
    , m_matrix(matrix)
{
}

SVGTransform::~SVGTransform()
{
}

PassRefPtr<SVGTransform> SVGTransform::clone() const
{
    return adoptRef(new SVGTransform(m_transformType, m_angle, m_center, m_matrix));
}

PassRefPtr<SVGPropertyBase> SVGTransform::cloneForAnimation(const String&) const
{
    // SVGTransform is never animated.
    ASSERT_NOT_REACHED();
    return nullptr;
}

void SVGTransform::setMatrix(const AffineTransform& matrix)
{
    onMatrixChange();
    m_matrix = matrix;
}

void SVGTransform::onMatrixChange()
{
    m_transformType = SVG_TRANSFORM_MATRIX;
    m_angle = 0;
}

void SVGTransform::setTranslate(float tx, float ty)
{
    m_transformType = SVG_TRANSFORM_TRANSLATE;
    m_angle = 0;

    m_matrix.makeIdentity();
    m_matrix.translate(tx, ty);
}

FloatPoint SVGTransform::translate() const
{
    return FloatPoint::narrowPrecision(m_matrix.e(), m_matrix.f());
}

void SVGTransform::setScale(float sx, float sy)
{
    m_transformType = SVG_TRANSFORM_SCALE;
    m_angle = 0;
    m_center = FloatPoint();

    m_matrix.makeIdentity();
    m_matrix.scaleNonUniform(sx, sy);
}

FloatSize SVGTransform::scale() const
{
    return FloatSize::narrowPrecision(m_matrix.a(), m_matrix.d());
}

void SVGTransform::setRotate(float angle, float cx, float cy)
{
    m_transformType = SVG_TRANSFORM_ROTATE;
    m_angle = angle;
    m_center = FloatPoint(cx, cy);

    // TODO: toString() implementation, which can show cx, cy (need to be stored?)
    m_matrix.makeIdentity();
    m_matrix.translate(cx, cy);
    m_matrix.rotate(angle);
    m_matrix.translate(-cx, -cy);
}

void SVGTransform::setSkewX(float angle)
{
    m_transformType = SVG_TRANSFORM_SKEWX;
    m_angle = angle;

    m_matrix.makeIdentity();
    m_matrix.skewX(angle);
}

void SVGTransform::setSkewY(float angle)
{
    m_transformType = SVG_TRANSFORM_SKEWY;
    m_angle = angle;

    m_matrix.makeIdentity();
    m_matrix.skewY(angle);
}

namespace {

const String& transformTypePrefixForParsing(SVGTransformType type)
{
    switch (type) {
    case SVG_TRANSFORM_UNKNOWN:
        return emptyString();
    case SVG_TRANSFORM_MATRIX: {
        DEFINE_STATIC_LOCAL(String, matrixString, ("matrix("));
        return matrixString;
    }
    case SVG_TRANSFORM_TRANSLATE: {
        DEFINE_STATIC_LOCAL(String, translateString, ("translate("));
        return translateString;
    }
    case SVG_TRANSFORM_SCALE: {
        DEFINE_STATIC_LOCAL(String, scaleString, ("scale("));
        return scaleString;
    }
    case SVG_TRANSFORM_ROTATE: {
        DEFINE_STATIC_LOCAL(String, rotateString, ("rotate("));
        return rotateString;
    }
    case SVG_TRANSFORM_SKEWX: {
        DEFINE_STATIC_LOCAL(String, skewXString, ("skewX("));
        return skewXString;
    }
    case SVG_TRANSFORM_SKEWY: {
        DEFINE_STATIC_LOCAL(String, skewYString, ("skewY("));
        return skewYString;
    }
    }

    ASSERT_NOT_REACHED();
    return emptyString();
}

}

String SVGTransform::valueAsString() const
{
    const String& prefix = transformTypePrefixForParsing(m_transformType);
    switch (m_transformType) {
    case SVG_TRANSFORM_UNKNOWN:
        return prefix;
    case SVG_TRANSFORM_MATRIX: {
        StringBuilder builder;
        builder.append(prefix + String::number(m_matrix.a()) + ' ' + String::number(m_matrix.b()) + ' ' + String::number(m_matrix.c()) + ' ' +
                       String::number(m_matrix.d()) + ' ' + String::number(m_matrix.e()) + ' ' + String::number(m_matrix.f()) + ')');
        return builder.toString();
    }
    case SVG_TRANSFORM_TRANSLATE:
        return prefix + String::number(m_matrix.e()) + ' ' + String::number(m_matrix.f()) + ')';
    case SVG_TRANSFORM_SCALE:
        return prefix + String::number(m_matrix.xScale()) + ' ' + String::number(m_matrix.yScale()) + ')';
    case SVG_TRANSFORM_ROTATE: {
        double angleInRad = deg2rad(m_angle);
        double cosAngle = cos(angleInRad);
        double sinAngle = sin(angleInRad);
        float cx = narrowPrecisionToFloat(cosAngle != 1 ? (m_matrix.e() * (1 - cosAngle) - m_matrix.f() * sinAngle) / (1 - cosAngle) / 2 : 0);
        float cy = narrowPrecisionToFloat(cosAngle != 1 ? (m_matrix.e() * sinAngle / (1 - cosAngle) + m_matrix.f()) / 2 : 0);
        if (cx || cy)
            return prefix + String::number(m_angle) + ' ' + String::number(cx) + ' ' + String::number(cy) + ')';
        return prefix + String::number(m_angle) + ')';
    }
    case SVG_TRANSFORM_SKEWX:
        return prefix + String::number(m_angle) + ')';
    case SVG_TRANSFORM_SKEWY:
        return prefix + String::number(m_angle) + ')';
    }

    ASSERT_NOT_REACHED();
    return emptyString();
}

void SVGTransform::add(PassRefPtrWillBeRawPtr<SVGPropertyBase>, SVGElement*)
{
    // SVGTransform is not animated by itself.
    ASSERT_NOT_REACHED();
}

void SVGTransform::calculateAnimatedValue(SVGAnimationElement*, float, unsigned, PassRefPtr<SVGPropertyBase>, PassRefPtr<SVGPropertyBase>, PassRefPtr<SVGPropertyBase>, SVGElement*)
{
    // SVGTransform is not animated by itself.
    ASSERT_NOT_REACHED();
}

float SVGTransform::calculateDistance(PassRefPtr<SVGPropertyBase>, SVGElement*)
{
    // SVGTransform is not animated by itself.
    ASSERT_NOT_REACHED();

    return -1;
}

} // namespace WebCore
