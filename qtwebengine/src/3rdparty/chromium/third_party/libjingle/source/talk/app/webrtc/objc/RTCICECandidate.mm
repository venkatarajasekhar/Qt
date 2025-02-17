/*
 * libjingle
 * Copyright 2013, Google Inc.
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

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "RTCICECandidate+Internal.h"

@implementation RTCICECandidate

@synthesize sdpMid = _sdpMid;
@synthesize sdpMLineIndex = _sdpMLineIndex;
@synthesize sdp = _sdp;

- (id)initWithMid:(NSString*)sdpMid
            index:(NSInteger)sdpMLineIndex
              sdp:(NSString*)sdp {
  if (!sdpMid || !sdp) {
    NSAssert(NO, @"nil arguments not allowed");
    return nil;
  }
  if ((self = [super init])) {
    _sdpMid = [sdpMid copy];
    _sdpMLineIndex = sdpMLineIndex;
    _sdp = [sdp copy];
  }
  return self;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"%@:%ld:%@",
                                    self.sdpMid,
                                    (long)self.sdpMLineIndex,
                                    self.sdp];
}

@end

@implementation RTCICECandidate (Internal)

- (id)initWithCandidate:(const webrtc::IceCandidateInterface*)candidate {
  if ((self = [super init])) {
    std::string sdp;
    if (candidate->ToString(&sdp)) {
      _sdpMid = @(candidate->sdp_mid().c_str());
      _sdpMLineIndex = candidate->sdp_mline_index();
      _sdp = @(sdp.c_str());
    } else {
      self = nil;
      NSAssert(NO, @"ICECandidateInterface->ToString failed");
    }
  }
  return self;
}

- (const webrtc::IceCandidateInterface*)candidate {
  return webrtc::CreateIceCandidate(
      [self.sdpMid UTF8String], self.sdpMLineIndex, [self.sdp UTF8String]);
}

@end
