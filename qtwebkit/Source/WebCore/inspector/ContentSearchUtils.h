/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ContentSearchUtils_h
#define ContentSearchUtils_h

#if ENABLE(INSPECTOR)

#include "InspectorTypeBuilder.h"
#include <wtf/Vector.h>
#include <wtf/text/TextPosition.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorArray;
class RegularExpression;

namespace ContentSearchUtils {

RegularExpression createSearchRegex(const String& query, bool caseSensitive, bool isRegex);
int countRegularExpressionMatches(const RegularExpression&, const String&);
PassRefPtr<TypeBuilder::Array<TypeBuilder::Page::SearchMatch> > searchInTextByLines(const String& text, const String& query, const bool caseSensitive, const bool isRegex);
TextPosition textPositionFromOffset(size_t offset, const Vector<size_t>& lineEndings);
PassOwnPtr<Vector<size_t> > lineEndings(const String&);

String findScriptSourceURL(const String& content);
String findScriptSourceMapURL(const String& content);
String findStylesheetSourceMapURL(const String& content);

} // namespace ContentSearchUtils
} // namespace WebCore

#endif // ENABLE(INSPECTOR)

#endif // !defined(ContentSearchUtils_h)
