/*
 * libjingle
 * Copyright 2014, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "RTCDataChannel.h"

#include "talk/app/webrtc/datachannelinterface.h"
#include "talk/base/scoped_ref_ptr.h"

@interface RTCDataBuffer (Internal)

@property(nonatomic, readonly) const webrtc::DataBuffer* dataBuffer;

- (instancetype)initWithDataBuffer:(const webrtc::DataBuffer&)buffer;

@end

@interface RTCDataChannelInit (Internal)

@property(nonatomic, readonly) const webrtc::DataChannelInit* dataChannelInit;

@end

@interface RTCDataChannel (Internal)

@property(nonatomic, readonly)
    talk_base::scoped_refptr<webrtc::DataChannelInterface> dataChannel;

- (instancetype)initWithDataChannel:
        (talk_base::scoped_refptr<webrtc::DataChannelInterface>)dataChannel;

@end
