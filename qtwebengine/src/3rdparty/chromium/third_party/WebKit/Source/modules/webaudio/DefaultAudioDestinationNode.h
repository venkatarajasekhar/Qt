/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#ifndef DefaultAudioDestinationNode_h
#define DefaultAudioDestinationNode_h

#include "platform/audio/AudioDestination.h"
#include "modules/webaudio/AudioDestinationNode.h"
#include "wtf/OwnPtr.h"

namespace WebCore {

class AudioContext;
class ExceptionState;

class DefaultAudioDestinationNode FINAL : public AudioDestinationNode {
public:
    static PassRefPtrWillBeRawPtr<DefaultAudioDestinationNode> create(AudioContext* context)
    {
        return adoptRefWillBeNoop(new DefaultAudioDestinationNode(context));
    }

    virtual ~DefaultAudioDestinationNode();

    // AudioNode
    virtual void initialize() OVERRIDE;
    virtual void uninitialize() OVERRIDE;
    virtual void setChannelCount(unsigned long, ExceptionState&) OVERRIDE;

    // AudioDestinationNode
    virtual void startRendering() OVERRIDE;
    virtual unsigned long maxChannelCount() const OVERRIDE;

private:
    explicit DefaultAudioDestinationNode(AudioContext*);
    void createDestination();

    OwnPtr<AudioDestination> m_destination;
    String m_inputDeviceId;
    unsigned m_numberOfInputChannels;
};

} // namespace WebCore

#endif // DefaultAudioDestinationNode_h
