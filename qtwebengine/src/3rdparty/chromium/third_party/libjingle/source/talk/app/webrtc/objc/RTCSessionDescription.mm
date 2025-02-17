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

#import "RTCSessionDescription+Internal.h"

@implementation RTCSessionDescription

@synthesize description = _description;
@synthesize type = _type;

- (id)initWithType:(NSString*)type sdp:(NSString*)sdp {
  if (!type || !sdp) {
    NSAssert(NO, @"nil arguments not allowed");
    return nil;
  }
  if ((self = [super init])) {
    _description = sdp;
    _type = type;
  }
  return self;
}

@end

@implementation RTCSessionDescription (Internal)

- (id)initWithSessionDescription:
          (const webrtc::SessionDescriptionInterface*)sessionDescription {
  if (!sessionDescription) {
    NSAssert(NO, @"nil arguments not allowed");
    self = nil;
    return nil;
  }
  if ((self = [super init])) {
    const std::string& type = sessionDescription->type();
    std::string sdp;
    if (!sessionDescription->ToString(&sdp)) {
      NSAssert(NO, @"Invalid SessionDescriptionInterface.");
      self = nil;
    } else {
      _description = @(sdp.c_str());
      _type = @(type.c_str());
    }
  }
  return self;
}

- (webrtc::SessionDescriptionInterface*)sessionDescription {
  return webrtc::CreateSessionDescription(
      [self.type UTF8String], [self.description UTF8String], NULL);
}

@end
