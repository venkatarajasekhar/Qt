/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "modules/websockets/WebSocketExtensionParser.h"

#include "wtf/ASCIICType.h"
#include "wtf/text/CString.h"

namespace WebCore {

WebSocketExtensionParser::ParserStateBackup::~ParserStateBackup()
{
    if (!m_isDisposed) {
        m_parser->m_current = m_current;
    }
}

bool WebSocketExtensionParser::finished()
{
    return m_current >= m_end;
}

bool WebSocketExtensionParser::parsedSuccessfully()
{
    return m_current == m_end;
}

static bool isSeparator(char character)
{
    static const char* separatorCharacters = "()<>@,;:\\\"/[]?={} \t";
    const char* p = strchr(separatorCharacters, character);
    return p && *p;
}

void WebSocketExtensionParser::skipSpaces()
{
    while (m_current < m_end && (*m_current == ' ' || *m_current == '\t'))
        ++m_current;
}

bool WebSocketExtensionParser::consumeToken()
{
    ParserStateBackup backup(this);
    skipSpaces();
    const char* start = m_current;
    while (m_current < m_end && isASCIIPrintable(*m_current) && !isSeparator(*m_current))
        ++m_current;
    if (start < m_current) {
        m_currentToken = String(start, m_current - start);
        backup.dispose();
        return true;
    }
    return false;
}

bool WebSocketExtensionParser::consumeQuotedString()
{
    ParserStateBackup backup(this);
    skipSpaces();
    if (m_current >= m_end || *m_current != '"')
        return false;

    Vector<char> buffer;
    ++m_current;
    while (m_current < m_end && *m_current != '"') {
        if (*m_current == '\\' && ++m_current >= m_end)
            return false;
        // RFC6455 requires that the value after quoted-string unescaping
        // MUST conform to the 'token' ABNF.
        if (!isASCIIPrintable(*m_current) || isSeparator(*m_current))
            return false;
        buffer.append(*m_current);
        ++m_current;
    }
    if (m_current >= m_end || *m_current != '"' || buffer.isEmpty())
        return false;
    m_currentToken = String::fromUTF8(buffer.data(), buffer.size());
    ++m_current;
    backup.dispose();
    return true;
}

bool WebSocketExtensionParser::consumeQuotedStringOrToken()
{
    // This is ok because consumeQuotedString() restores m_current
    // on failure.
    return consumeQuotedString() || consumeToken();
}

bool WebSocketExtensionParser::consumeCharacter(char character)
{
    ParserStateBackup backup(this);
    skipSpaces();
    if (m_current < m_end && *m_current == character) {
        ++m_current;
        backup.dispose();
        return true;
    }
    return false;
}

bool WebSocketExtensionParser::parseExtension(String& extensionToken, HashMap<String, String>& extensionParameters)
{
    ParserStateBackup backup(this);
    // Parse extension-token.
    if (!consumeToken())
        return false;

    extensionToken = currentToken();

    // Parse extension-parameters if exists.
    while (consumeCharacter(';')) {
        if (!consumeToken())
            return false;

        String parameterToken = currentToken();
        if (consumeCharacter('=')) {
            if (consumeQuotedStringOrToken())
                extensionParameters.add(parameterToken, currentToken());
            else
                return false;
        } else {
            extensionParameters.add(parameterToken, String());
        }
    }
    skipSpaces();
    if (!finished() && !consumeCharacter(','))
        return false;

    backup.dispose();
    return true;
}

} // namespace WebCore
