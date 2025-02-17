/*
 * Copyright (C) 2013 Apple Computer, Inc.  All rights reserved.
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

#ifndef PlatformSpeechSynthesisUtterance_h
#define PlatformSpeechSynthesisUtterance_h

#include "platform/PlatformExport.h"
#include "platform/heap/Handle.h"
#include "platform/speech/PlatformSpeechSynthesisVoice.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class PlatformSpeechSynthesisUtteranceClient : public GarbageCollectedMixin {
public:
    // Implement methods as needed.
protected:
    virtual ~PlatformSpeechSynthesisUtteranceClient() { }
};

class PLATFORM_EXPORT PlatformSpeechSynthesisUtterance FINAL : public GarbageCollectedFinalized<PlatformSpeechSynthesisUtterance> {
public:
    static PlatformSpeechSynthesisUtterance* create(PlatformSpeechSynthesisUtteranceClient*);

    const String& text() const { return m_text; }
    void setText(const String& text) { m_text = text; }

    const String& lang() const { return m_lang; }
    void setLang(const String& lang) { m_lang = lang; }

    PlatformSpeechSynthesisVoice* voice() const { return m_voice; }
    void setVoice(PlatformSpeechSynthesisVoice* voice) { m_voice = voice; }

    // Range = [0, 1] where 1 is the default.
    float volume() const { return m_volume; }
    void setVolume(float volume) { m_volume = std::max(std::min(1.0f, volume), 0.0f); }

    // Range = [0.1, 10] where 1 is the default.
    float rate() const { return m_rate; }
    void setRate(float rate) { m_rate = std::max(std::min(10.0f, rate), 0.1f); }

    // Range = [0, 2] where 1 is the default.
    float pitch() const { return m_pitch; }
    void setPitch(float pitch) { m_pitch = std::max(std::min(2.0f, pitch), 0.0f); }

    double startTime() const { return m_startTime; }
    void setStartTime(double startTime) { m_startTime = startTime; }

    PlatformSpeechSynthesisUtteranceClient* client() const { return m_client; }

    void trace(Visitor*);

private:
    explicit PlatformSpeechSynthesisUtterance(PlatformSpeechSynthesisUtteranceClient*);

    Member<PlatformSpeechSynthesisUtteranceClient> m_client;
    String m_text;
    String m_lang;
    Member<PlatformSpeechSynthesisVoice> m_voice;
    float m_volume;
    float m_rate;
    float m_pitch;
    double m_startTime;
};

} // namespace WebCore

#endif // PlatformSpeechSynthesisUtterance_h
