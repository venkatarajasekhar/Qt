/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ConvolverNode_h
#define ConvolverNode_h

#include "modules/webaudio/AudioNode.h"
#include "wtf/OwnPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/ThreadingPrimitives.h"

namespace WebCore {

class AudioBuffer;
class ExceptionState;
class Reverb;

class ConvolverNode FINAL : public AudioNode {
public:
    static PassRefPtrWillBeRawPtr<ConvolverNode> create(AudioContext* context, float sampleRate)
    {
        return adoptRefWillBeNoop(new ConvolverNode(context, sampleRate));
    }

    virtual ~ConvolverNode();

    // AudioNode
    virtual void process(size_t framesToProcess) OVERRIDE;
    virtual void initialize() OVERRIDE;
    virtual void uninitialize() OVERRIDE;

    // Impulse responses
    void setBuffer(AudioBuffer*, ExceptionState&);
    AudioBuffer* buffer();

    bool normalize() const { return m_normalize; }
    void setNormalize(bool normalize) { m_normalize = normalize; }

    virtual void trace(Visitor*) OVERRIDE;

private:
    ConvolverNode(AudioContext*, float sampleRate);

    virtual double tailTime() const OVERRIDE;
    virtual double latencyTime() const OVERRIDE;

    OwnPtr<Reverb> m_reverb;
    RefPtrWillBeMember<AudioBuffer> m_buffer;

    // This synchronizes dynamic changes to the convolution impulse response with process().
    mutable Mutex m_processLock;

    // Normalize the impulse response or not. Must default to true.
    bool m_normalize;
};

} // namespace WebCore

#endif // ConvolverNode_h
