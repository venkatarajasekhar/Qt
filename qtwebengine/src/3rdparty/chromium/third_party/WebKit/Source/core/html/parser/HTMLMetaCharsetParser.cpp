/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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
#include "core/html/parser/HTMLMetaCharsetParser.h"

#include "core/HTMLNames.h"
#include "core/html/parser/HTMLParserIdioms.h"
#include "core/html/parser/HTMLParserOptions.h"
#include "core/html/parser/HTMLTokenizer.h"
#include "wtf/text/TextEncodingRegistry.h"
#include "wtf/text/WTFString.h"

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

HTMLMetaCharsetParser::HTMLMetaCharsetParser()
    : m_tokenizer(HTMLTokenizer::create(HTMLParserOptions(0)))
    , m_assumedCodec(newTextCodec(Latin1Encoding()))
    , m_inHeadSection(true)
    , m_doneChecking(false)
{
}

HTMLMetaCharsetParser::~HTMLMetaCharsetParser()
{
}

bool HTMLMetaCharsetParser::processMeta()
{
    const HTMLToken::AttributeList& tokenAttributes = m_token.attributes();
    HTMLAttributeList attributes;
    for (HTMLToken::AttributeList::const_iterator iter = tokenAttributes.begin(); iter != tokenAttributes.end(); ++iter) {
        String attributeName = attemptStaticStringCreation(iter->name, Likely8Bit);
        String attributeValue = StringImpl::create8BitIfPossible(iter->value);
        attributes.append(std::make_pair(attributeName, attributeValue));
    }

    m_encoding = encodingFromMetaAttributes(attributes);
    return m_encoding.isValid();
}

static const int bytesToCheckUnconditionally = 1024; // That many input bytes will be checked for meta charset even if <head> section is over.

bool HTMLMetaCharsetParser::checkForMetaCharset(const char* data, size_t length)
{
    if (m_doneChecking)
        return true;

    ASSERT(!m_encoding.isValid());

    // We still don't have an encoding, and are in the head.
    // The following tags are allowed in <head>:
    // SCRIPT|STYLE|META|LINK|OBJECT|TITLE|BASE

    // We stop scanning when a tag that is not permitted in <head>
    // is seen, rather when </head> is seen, because that more closely
    // matches behavior in other browsers; more details in
    // <http://bugs.webkit.org/show_bug.cgi?id=3590>.

    // Additionally, we ignore things that looks like tags in <title>, <script>
    // and <noscript>; see <http://bugs.webkit.org/show_bug.cgi?id=4560>,
    // <http://bugs.webkit.org/show_bug.cgi?id=12165> and
    // <http://bugs.webkit.org/show_bug.cgi?id=12389>.

    // Since many sites have charset declarations after <body> or other tags
    // that are disallowed in <head>, we don't bail out until we've checked at
    // least bytesToCheckUnconditionally bytes of input.

    m_input.append(SegmentedString(m_assumedCodec->decode(data, length)));

    while (m_tokenizer->nextToken(m_input, m_token)) {
        bool end = m_token.type() == HTMLToken::EndTag;
        if (end || m_token.type() == HTMLToken::StartTag) {
            String tagName = attemptStaticStringCreation(m_token.name(), Likely8Bit);
            if (!end) {
                m_tokenizer->updateStateFor(tagName);
                if (threadSafeMatch(tagName, metaTag) && processMeta()) {
                    m_doneChecking = true;
                    return true;
                }
            }

            if (!threadSafeMatch(tagName, scriptTag) && !threadSafeMatch(tagName, noscriptTag)
                && !threadSafeMatch(tagName, styleTag) && !threadSafeMatch(tagName, linkTag)
                && !threadSafeMatch(tagName, metaTag) && !threadSafeMatch(tagName, objectTag)
                && !threadSafeMatch(tagName, titleTag) && !threadSafeMatch(tagName, baseTag)
                && (end || !threadSafeMatch(tagName, htmlTag)) && (end || !threadSafeMatch(tagName, headTag))) {
                m_inHeadSection = false;
            }
        }

        if (!m_inHeadSection && m_input.numberOfCharactersConsumed() >= bytesToCheckUnconditionally) {
            m_doneChecking = true;
            return true;
        }

        m_token.clear();
    }

    return false;
}

}
