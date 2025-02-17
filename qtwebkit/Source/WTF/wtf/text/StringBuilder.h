/*
 * Copyright (C) 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef StringBuilder_h
#define StringBuilder_h

#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

namespace WTF {

class StringBuilder {
    // Disallow copying since it's expensive and we don't want code to do it by accident.
    WTF_MAKE_NONCOPYABLE(StringBuilder);

public:
    StringBuilder()
        : m_length(0)
        , m_is8Bit(true)
        , m_valid16BitShadowLength(0)
        , m_bufferCharacters8(0)
    {
    }

    WTF_EXPORT_PRIVATE void append(const UChar*, unsigned);
    WTF_EXPORT_PRIVATE void append(const LChar*, unsigned);

    ALWAYS_INLINE void append(const char* characters, unsigned length) { append(reinterpret_cast<const LChar*>(characters), length); }

    void append(const String& string)
    {
        if (!string.length())
            return;

        // If we're appending to an empty string, and there is not a buffer (reserveCapacity has not been called)
        // then just retain the string.
        if (!m_length && !m_buffer) {
            m_string = string;
            m_length = string.length();
            m_is8Bit = m_string.is8Bit();
            return;
        }

        if (string.is8Bit())
            append(string.characters8(), string.length());
        else
            append(string.characters16(), string.length());
    }

    void append(const StringBuilder& other)
    {
        if (!other.m_length)
            return;

        // If we're appending to an empty string, and there is not a buffer (reserveCapacity has not been called)
        // then just retain the string.
        if (!m_length && !m_buffer && !other.m_string.isNull()) {
            m_string = other.m_string;
            m_length = other.m_length;
            return;
        }

        if (other.is8Bit())
            append(other.characters8(), other.m_length);
        else
            append(other.characters16(), other.m_length);
    }

    void append(const String& string, unsigned offset, unsigned length)
    {
        if (!string.length())
            return;

        if ((offset + length) > string.length())
            return;

        if (string.is8Bit())
            append(string.characters8() + offset, length);
        else
            append(string.characters16() + offset, length);
    }

    void append(const char* characters)
    {
        if (characters)
            append(characters, strlen(characters));
    }

    void append(UChar c)
    {
        if (m_buffer && m_length < m_buffer->length() && m_string.isNull()) {
            if (!m_is8Bit) {
                m_bufferCharacters16[m_length++] = c;
                return;
            }

            if (!(c & ~0xff)) {
                m_bufferCharacters8[m_length++] = static_cast<LChar>(c);
                return;
            }
        }
        append(&c, 1);
    }

    void append(LChar c)
    {
        if (m_buffer && m_length < m_buffer->length() && m_string.isNull()) {
            if (m_is8Bit)
                m_bufferCharacters8[m_length++] = c;
            else
                m_bufferCharacters16[m_length++] = c;
        } else
            append(&c, 1);
    }

    void append(char c)
    {
        append(static_cast<LChar>(c));
    }

    void append(UChar32 c)
    {
        if (U_IS_BMP(c)) {
            append(static_cast<UChar>(c));
            return;
        }
        append(U16_LEAD(c));
        append(U16_TRAIL(c));
    }

    template<unsigned charactersCount>
    ALWAYS_INLINE void appendLiteral(const char (&characters)[charactersCount]) { append(characters, charactersCount - 1); }

    WTF_EXPORT_PRIVATE void appendNumber(int);
    WTF_EXPORT_PRIVATE void appendNumber(unsigned int);
    WTF_EXPORT_PRIVATE void appendNumber(long);
    WTF_EXPORT_PRIVATE void appendNumber(unsigned long);
    WTF_EXPORT_PRIVATE void appendNumber(long long);
    WTF_EXPORT_PRIVATE void appendNumber(unsigned long long);

    String toString()
    {
        shrinkToFit();
        if (m_string.isNull())
            reifyString();
        return m_string;
    }

    const String& toStringPreserveCapacity() const
    {
        if (m_string.isNull())
            reifyString();
        return m_string;
    }

    AtomicString toAtomicString() const
    {
        if (!m_length)
            return emptyAtom;

        // If the buffer is sufficiently over-allocated, make a new AtomicString from a copy so its buffer is not so large.
        if (canShrink()) {
            if (is8Bit())
                return AtomicString(characters8(), length());
            return AtomicString(characters16(), length());            
        }

        if (!m_string.isNull())
            return AtomicString(m_string);

        ASSERT(m_buffer);
        return AtomicString(m_buffer.get(), 0, m_length);
    }

    unsigned length() const
    {
        return m_length;
    }

    bool isEmpty() const { return !m_length; }

    WTF_EXPORT_PRIVATE void reserveCapacity(unsigned newCapacity);

    unsigned capacity() const
    {
        return m_buffer ? m_buffer->length() : m_length;
    }

    WTF_EXPORT_PRIVATE void resize(unsigned newSize);

    WTF_EXPORT_PRIVATE bool canShrink() const;

    WTF_EXPORT_PRIVATE void shrinkToFit();

    UChar operator[](unsigned i) const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_length);
        if (m_is8Bit)
            return characters8()[i];
        return characters16()[i];
    }

    const LChar* characters8() const
    {
        ASSERT(m_is8Bit);
        if (!m_length)
            return 0;
        if (!m_string.isNull())
            return m_string.characters8();
        ASSERT(m_buffer);
        return m_buffer->characters8();
    }

    const UChar* characters16() const
    {
        ASSERT(!m_is8Bit);
        if (!m_length)
            return 0;
        if (!m_string.isNull())
            return m_string.characters16();
        ASSERT(m_buffer);
        return m_buffer->characters16();
    }
    
    const UChar* characters() const
    {
        if (!m_length)
            return 0;
        if (!m_string.isNull())
            return m_string.characters();
        ASSERT(m_buffer);
        if (m_buffer->has16BitShadow() && m_valid16BitShadowLength < m_length)
            m_buffer->upconvertCharacters(m_valid16BitShadowLength, m_length);

        m_valid16BitShadowLength = m_length;

        return m_buffer->characters();
    }
    
    bool is8Bit() const { return m_is8Bit; }

    void clear()
    {
        m_length = 0;
        m_string = String();
        m_buffer = 0;
        m_bufferCharacters8 = 0;
        m_is8Bit = true;
        m_valid16BitShadowLength = 0;
    }

    void swap(StringBuilder& stringBuilder)
    {
        std::swap(m_length, stringBuilder.m_length);
        m_string.swap(stringBuilder.m_string);
        m_buffer.swap(stringBuilder.m_buffer);
        std::swap(m_is8Bit, stringBuilder.m_is8Bit);
        std::swap(m_valid16BitShadowLength, stringBuilder.m_valid16BitShadowLength);
        std::swap(m_bufferCharacters8, stringBuilder.m_bufferCharacters8);
    }

private:
    void allocateBuffer(const LChar* currentCharacters, unsigned requiredLength);
    void allocateBuffer(const UChar* currentCharacters, unsigned requiredLength);
    void allocateBufferUpConvert(const LChar* currentCharacters, unsigned requiredLength);
    template <typename CharType>
    void reallocateBuffer(unsigned requiredLength);
    template <typename CharType>
    ALWAYS_INLINE CharType* appendUninitialized(unsigned length);
    template <typename CharType>
    CharType* appendUninitializedSlow(unsigned length);
    template <typename CharType>
    ALWAYS_INLINE CharType * getBufferCharacters();
    WTF_EXPORT_PRIVATE void reifyString() const;

    unsigned m_length;
    mutable String m_string;
    RefPtr<StringImpl> m_buffer;
    bool m_is8Bit;
    mutable unsigned m_valid16BitShadowLength;
    union {
        LChar* m_bufferCharacters8;
        UChar* m_bufferCharacters16;
    };
};

template <>
ALWAYS_INLINE LChar* StringBuilder::getBufferCharacters<LChar>()
{
    ASSERT(m_is8Bit);
    return m_bufferCharacters8;
}

template <>
ALWAYS_INLINE UChar* StringBuilder::getBufferCharacters<UChar>()
{
    ASSERT(!m_is8Bit);
    return m_bufferCharacters16;
}    

template <typename CharType>
bool equal(const StringBuilder& s, const CharType* buffer, unsigned length)
{
    if (s.length() != length)
        return false;

    if (s.is8Bit())
        return equal(s.characters8(), buffer, length);

    return equal(s.characters16(), buffer, length);
}

template <typename StringType>
bool equal(const StringBuilder& a, const StringType& b)
{
    if (a.length() != b.length())
        return false;

    if (!a.length())
        return true;

    if (a.is8Bit()) {
        if (b.is8Bit())
            return equal(a.characters8(), b.characters8(), a.length());
        return equal(a.characters8(), b.characters16(), a.length());
    }

    if (b.is8Bit())
        return equal(a.characters16(), b.characters8(), a.length());
    return equal(a.characters16(), b.characters16(), a.length());
}

inline bool operator==(const StringBuilder& a, const StringBuilder& b) { return equal(a, b); }
inline bool operator!=(const StringBuilder& a, const StringBuilder& b) { return !equal(a, b); }
inline bool operator==(const StringBuilder& a, const String& b) { return equal(a, b); }
inline bool operator!=(const StringBuilder& a, const String& b) { return !equal(a, b); }
inline bool operator==(const String& a, const StringBuilder& b) { return equal(b, a); }
inline bool operator!=(const String& a, const StringBuilder& b) { return !equal(b, a); }

} // namespace WTF

using WTF::StringBuilder;

#endif // StringBuilder_h
