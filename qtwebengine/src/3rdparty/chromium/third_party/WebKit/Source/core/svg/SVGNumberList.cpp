/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "core/svg/SVGNumberList.h"

#include "core/svg/SVGAnimationElement.h"
#include "core/svg/SVGParserUtilities.h"
#include "wtf/text/StringBuilder.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

inline PassRefPtr<SVGNumberList> toSVGNumberList(PassRefPtr<SVGPropertyBase> passBase)
{
    RefPtr<SVGPropertyBase> base = passBase;
    ASSERT(base->type() == SVGNumberList::classType());
    return static_pointer_cast<SVGNumberList>(base.release());
}

SVGNumberList::SVGNumberList()
{
}

SVGNumberList::~SVGNumberList()
{
}

PassRefPtr<SVGNumberList> SVGNumberList::clone()
{
    RefPtr<SVGNumberList> svgNumberList = SVGNumberList::create();
    svgNumberList->deepCopy(this);
    return svgNumberList.release();
}

PassRefPtr<SVGPropertyBase> SVGNumberList::cloneForAnimation(const String& value) const
{
    RefPtr<SVGNumberList> svgNumberList = SVGNumberList::create();
    svgNumberList->setValueAsString(value, IGNORE_EXCEPTION);
    return svgNumberList.release();
}

String SVGNumberList::valueAsString() const
{
    StringBuilder builder;

    ConstIterator it = begin();
    ConstIterator itEnd = end();
    if (it != itEnd) {
        builder.append(it->valueAsString());
        ++it;

        for (; it != itEnd; ++it) {
            builder.append(' ');
            builder.append(it->valueAsString());
        }
    }

    return builder.toString();
}

template <typename CharType>
bool SVGNumberList::parse(const CharType*& ptr, const CharType* end)
{
    clear();

    while (ptr < end) {
        float number = 0;
        if (!parseNumber(ptr, end, number))
            return false;
        append(SVGNumber::create(number));
    }

    return true;
}

void SVGNumberList::setValueAsString(const String& value, ExceptionState& exceptionState)
{
    if (value.isEmpty()) {
        clear();
        return;
    }

    bool valid = false;
    if (value.is8Bit()) {
        const LChar* ptr = value.characters8();
        const LChar* end = ptr + value.length();
        valid = parse(ptr, end);
    } else {
        const UChar* ptr = value.characters16();
        const UChar* end = ptr + value.length();
        valid = parse(ptr, end);
    }

    if (!valid) {
        exceptionState.throwDOMException(SyntaxError, "Problem parsing number list \""+value+"\"");
        // No call to |clear()| here. SVG policy is to use valid items before error.
        // Spec: http://www.w3.org/TR/SVG/single-page.html#implnote-ErrorProcessing
    }
}

void SVGNumberList::add(PassRefPtrWillBeRawPtr<SVGPropertyBase> other, SVGElement* contextElement)
{
    RefPtr<SVGNumberList> otherList = toSVGNumberList(other);

    if (length() != otherList->length())
        return;

    for (size_t i = 0; i < length(); ++i)
        at(i)->setValue(at(i)->value() + otherList->at(i)->value());
}

bool SVGNumberList::adjustFromToListValues(PassRefPtr<SVGNumberList> passFromList, PassRefPtr<SVGNumberList> passToList, float percentage, bool isToAnimation, bool resizeAnimatedListIfNeeded)
{
    RefPtr<SVGNumberList> fromList = passFromList;
    RefPtr<SVGNumberList> toList = passToList;

    // If no 'to' value is given, nothing to animate.
    size_t toListSize = toList->length();
    if (!toListSize)
        return false;

    // If the 'from' value is given and it's length doesn't match the 'to' value list length, fallback to a discrete animation.
    size_t fromListSize = fromList->length();
    if (fromListSize != toListSize && fromListSize) {
        if (percentage < 0.5) {
            if (!isToAnimation)
                deepCopy(fromList);
        } else {
            deepCopy(toList);
        }

        return false;
    }

    ASSERT(!fromListSize || fromListSize == toListSize);
    if (resizeAnimatedListIfNeeded && length() < toListSize) {
        size_t paddingCount = toListSize - length();
        for (size_t i = 0; i < paddingCount; ++i)
            append(SVGNumber::create());
    }

    return true;
}

void SVGNumberList::calculateAnimatedValue(SVGAnimationElement* animationElement, float percentage, unsigned repeatCount, PassRefPtr<SVGPropertyBase> fromValue, PassRefPtr<SVGPropertyBase> toValue, PassRefPtr<SVGPropertyBase> toAtEndOfDurationValue, SVGElement* contextElement)
{
    RefPtr<SVGNumberList> fromList = toSVGNumberList(fromValue);
    RefPtr<SVGNumberList> toList = toSVGNumberList(toValue);
    RefPtr<SVGNumberList> toAtEndOfDurationList = toSVGNumberList(toAtEndOfDurationValue);

    size_t fromListSize = fromList->length();
    size_t toListSize = toList->length();
    size_t toAtEndOfDurationListSize = toAtEndOfDurationList->length();

    if (!adjustFromToListValues(fromList, toList, percentage, animationElement->animationMode() == ToAnimation, true))
        return;

    for (size_t i = 0; i < toListSize; ++i) {
        float effectiveFrom = fromListSize ? fromList->at(i)->value() : 0;
        float effectiveTo = toListSize ? toList->at(i)->value() : 0;
        float effectiveToAtEnd = i < toAtEndOfDurationListSize ? toAtEndOfDurationList->at(i)->value() : 0;

        float animated = at(i)->value();
        animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom, effectiveTo, effectiveToAtEnd, animated);
        at(i)->setValue(animated);
    }
}

float SVGNumberList::calculateDistance(PassRefPtr<SVGPropertyBase> to, SVGElement*)
{
    // FIXME: Distance calculation is not possible for SVGNumberList right now. We need the distance for every single value.
    return -1;
}

Vector<float> SVGNumberList::toFloatVector() const
{
    Vector<float> vec;
    vec.reserveInitialCapacity(length());
    for (size_t i = 0; i < length(); ++i)
        vec.uncheckedAppend(at(i)->value());
    return vec;
}

}
