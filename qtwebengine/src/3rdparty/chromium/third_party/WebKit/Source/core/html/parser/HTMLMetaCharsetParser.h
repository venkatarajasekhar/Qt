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

#ifndef HTMLMetaCharsetParser_h
#define HTMLMetaCharsetParser_h

#include "core/html/parser/HTMLToken.h"
#include "platform/text/SegmentedString.h"
#include "wtf/Noncopyable.h"
#include "wtf/text/TextCodec.h"
#include "wtf/text/TextEncoding.h"

namespace WebCore {

class HTMLTokenizer;

class HTMLMetaCharsetParser {
    WTF_MAKE_NONCOPYABLE(HTMLMetaCharsetParser); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<HTMLMetaCharsetParser> create() { return adoptPtr(new HTMLMetaCharsetParser()); }

    ~HTMLMetaCharsetParser();

    // Returns true if done checking, regardless whether an encoding is found.
    bool checkForMetaCharset(const char*, size_t);

    const WTF::TextEncoding& encoding() { return m_encoding; }

private:
    HTMLMetaCharsetParser();

    bool processMeta();

    OwnPtr<HTMLTokenizer> m_tokenizer;
    OwnPtr<TextCodec> m_assumedCodec;
    SegmentedString m_input;
    HTMLToken m_token;
    bool m_inHeadSection;

    bool m_doneChecking;
    WTF::TextEncoding m_encoding;
};

}
#endif
