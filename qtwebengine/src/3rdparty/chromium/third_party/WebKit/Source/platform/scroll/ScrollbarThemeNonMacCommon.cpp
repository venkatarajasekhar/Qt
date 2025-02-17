/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2008, 2009 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "platform/scroll/ScrollbarThemeNonMacCommon.h"

#include "platform/PlatformMouseEvent.h"
#include "platform/graphics/GraphicsContextStateSaver.h"
#include "platform/scroll/ScrollableArea.h"
#include "platform/scroll/ScrollbarThemeClient.h"

namespace WebCore {

bool ScrollbarThemeNonMacCommon::hasThumb(ScrollbarThemeClient* scrollbar)
{
    // This method is just called as a paint-time optimization to see if
    // painting the thumb can be skipped. We don't have to be exact here.
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeNonMacCommon::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    // Windows and Linux just have single arrows.
    if (part == BackButtonEndPart)
        return IntRect();

    IntSize size = buttonSize(scrollbar);
    return IntRect(scrollbar->x(), scrollbar->y(), size.width(), size.height());
}

IntRect ScrollbarThemeNonMacCommon::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    // Windows and Linux just have single arrows.
    if (part == ForwardButtonStartPart)
        return IntRect();

    IntSize size = buttonSize(scrollbar);
    int x, y;
    if (scrollbar->orientation() == HorizontalScrollbar) {
        x = scrollbar->x() + scrollbar->width() - size.width();
        y = scrollbar->y();
    } else {
        x = scrollbar->x();
        y = scrollbar->y() + scrollbar->height() - size.height();
    }
    return IntRect(x, y, size.width(), size.height());
}

IntRect ScrollbarThemeNonMacCommon::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    IntSize bs = buttonSize(scrollbar);
    int thickness = scrollbarThickness(scrollbar->controlSize());
    if (scrollbar->orientation() == HorizontalScrollbar) {
        // Once the scrollbar becomes smaller than the size of the
        // two buttons with a 1 pixel gap, the track disappears.
        if (scrollbar->width() <= 2 * bs.width() + 1)
            return IntRect();
        return IntRect(scrollbar->x() + bs.width(), scrollbar->y(), scrollbar->width() - 2 * bs.width(), thickness);
    }
    if (scrollbar->height() <= 2 * bs.height() + 1)
        return IntRect();
    return IntRect(scrollbar->x(), scrollbar->y() + bs.height(), thickness, scrollbar->height() - 2 * bs.height());
}

void ScrollbarThemeNonMacCommon::paintTrackBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    // Just assume a forward track part. We only paint the track as a single piece when there is no thumb.
    if (!hasThumb(scrollbar))
        paintTrackPiece(context, scrollbar, rect, ForwardTrackPart);
}

void ScrollbarThemeNonMacCommon::paintTickmarks(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    if (scrollbar->orientation() != VerticalScrollbar)
        return;

    if (rect.height() <= 0 || rect.width() <= 0)
        return;

    // Get the tickmarks for the frameview.
    Vector<IntRect> tickmarks;
    scrollbar->getTickmarks(tickmarks);
    if (!tickmarks.size())
        return;

    GraphicsContextStateSaver stateSaver(*context);
    context->setShouldAntialias(false);

    for (Vector<IntRect>::const_iterator i = tickmarks.begin(); i != tickmarks.end(); ++i) {
        // Calculate how far down (in %) the tick-mark should appear.
        const float percent = static_cast<float>(i->y()) / scrollbar->totalSize();

        // Calculate how far down (in pixels) the tick-mark should appear.
        const int yPos = rect.y() + (rect.height() * percent);

        context->setFillColor(Color(0xCC, 0xAA, 0x00, 0xFF));
        FloatRect tickRect(rect.x(), yPos, rect.width(), 3);
        context->fillRect(tickRect);

        context->setFillColor(Color(0xFF, 0xDD, 0x00, 0xFF));
        FloatRect tickStroke(rect.x(), yPos + 1, rect.width(), 1);
        context->fillRect(tickStroke);
    }
}

} // namespace WebCore
