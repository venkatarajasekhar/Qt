/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CanvasPattern.h"

#include "ExceptionCode.h"
#include "Image.h"
#include "Pattern.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

PassRefPtr<CanvasPattern> CanvasPattern::create(PassRefPtr<Image> image, bool repeatX, bool repeatY, bool originClean)
{
    return adoptRef(new CanvasPattern(image, repeatX, repeatY, originClean));
}

CanvasPattern::CanvasPattern(PassRefPtr<Image> image, bool repeatX, bool repeatY, bool originClean)
    : m_pattern(Pattern::create(image, repeatX, repeatY))
    , m_originClean(originClean)
{
}

CanvasPattern::~CanvasPattern()
{
}

void CanvasPattern::parseRepetitionType(const String& type, bool& repeatX, bool& repeatY, ExceptionCode& ec)
{
    ec = 0;
    if (type.isEmpty() || type == "repeat") {
        repeatX = true;
        repeatY = true;
        return;
    }
    if (type == "no-repeat") {
        repeatX = false;
        repeatY = false;
        return;
    }
    if (type == "repeat-x") {
        repeatX = true;
        repeatY = false;
        return;
    }
    if (type == "repeat-y") {
        repeatX = false;
        repeatY = true;
        return;
    }
    ec = SYNTAX_ERR;
}

} // namespace WebCore
