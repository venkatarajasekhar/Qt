/*
 * Copyright (C) 2013, Opera Software ASA. All rights reserved.
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
#include "core/html/track/vtt/BufferedLineReader.h"

#include "wtf/unicode/CharacterNames.h"

namespace WebCore {

bool BufferedLineReader::getLine(String& line)
{
    if (m_maybeSkipLF) {
        // We ran out of data after a CR (U+000D), which means that we may be
        // in the middle of a CRLF pair. If the next character is a LF (U+000A)
        // then skip it, and then (unconditionally) return the buffered line.
        if (!m_buffer.isEmpty()) {
            scanCharacter(newlineCharacter);
            m_maybeSkipLF = false;
        }
        // If there was no (new) data available, then keep m_maybeSkipLF set,
        // and fall through all the way down to the EOS check at the end of
        // the method.
    }

    bool shouldReturnLine = false;
    bool checkForLF = false;
    while (!m_buffer.isEmpty()) {
        UChar c = m_buffer.currentChar();
        m_buffer.advance();

        if (c == newlineCharacter || c == carriageReturn) {
            // We found a line ending. Return the accumulated line.
            shouldReturnLine = true;
            checkForLF = (c == carriageReturn);
            break;
        }

        // NULs are transformed into U+FFFD (REPLACEMENT CHAR.) in step 1 of
        // the WebVTT parser algorithm.
        if (c == '\0')
            c = replacementCharacter;

        m_lineBuffer.append(c);
    }

    if (checkForLF) {
        // May be in the middle of a CRLF pair.
        if (!m_buffer.isEmpty()) {
            // Scan a potential newline character.
            scanCharacter(newlineCharacter);
        } else {
            // Check for the LF on the next call (unless we reached EOS, in
            // which case we'll return the contents of the line buffer, and
            // reset state for the next line.)
            m_maybeSkipLF = true;
        }
    }

    if (isAtEndOfStream()) {
        // We've reached the end of the stream proper. Emit a line if the
        // current line buffer is non-empty. (Note that if shouldReturnLine is
        // set already, we want to return a line nonetheless.)
        shouldReturnLine |= !m_lineBuffer.isEmpty();
    }

    if (shouldReturnLine) {
        line = m_lineBuffer.toString();
        m_lineBuffer.clear();
        return true;
    }

    ASSERT(m_buffer.isEmpty());
    return false;
}

} // namespace WebCore
