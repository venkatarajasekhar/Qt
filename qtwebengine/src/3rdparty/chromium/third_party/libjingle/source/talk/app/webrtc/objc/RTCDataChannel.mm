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

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "RTCDataChannel+Internal.h"

#include "talk/app/webrtc/datachannelinterface.h"

namespace webrtc {

class RTCDataChannelObserver : public DataChannelObserver {
 public:
  RTCDataChannelObserver(RTCDataChannel* channel) { _channel = channel; }

  virtual void OnStateChange() OVERRIDE {
    [_channel.delegate channelDidChangeState:_channel];
  }

  virtual void OnMessage(const DataBuffer& buffer) OVERRIDE {
    if (!_channel.delegate) {
      return;
    }
    RTCDataBuffer* dataBuffer =
        [[RTCDataBuffer alloc] initWithDataBuffer:buffer];
    [_channel.delegate channel:_channel didReceiveMessageWithBuffer:dataBuffer];
  }

 private:
  __weak RTCDataChannel* _channel;
};
}

// TODO(tkchin): move to shared location
NSString* NSStringFromStdString(const std::string& stdString) {
  // std::string may contain null termination character so we construct
  // using length.
  return [[NSString alloc] initWithBytes:stdString.data()
                                  length:stdString.length()
                                encoding:NSUTF8StringEncoding];
}

std::string StdStringFromNSString(NSString* nsString) {
  NSData* charData = [nsString dataUsingEncoding:NSUTF8StringEncoding];
  return std::string(reinterpret_cast<const char*>([charData bytes]),
                     [charData length]);
}

@implementation RTCDataChannelInit {
  webrtc::DataChannelInit _dataChannelInit;
}

- (BOOL)isOrdered {
  return _dataChannelInit.ordered;
}

- (void)setIsOrdered:(BOOL)isOrdered {
  _dataChannelInit.ordered = isOrdered;
}

- (NSInteger)maxRetransmitTime {
  return _dataChannelInit.maxRetransmitTime;
}

- (void)setMaxRetransmitTime:(NSInteger)maxRetransmitTime {
  _dataChannelInit.maxRetransmitTime = maxRetransmitTime;
}

- (NSInteger)maxRetransmits {
  return _dataChannelInit.maxRetransmits;
}

- (void)setMaxRetransmits:(NSInteger)maxRetransmits {
  _dataChannelInit.maxRetransmits = maxRetransmits;
}

- (NSString*)protocol {
  return NSStringFromStdString(_dataChannelInit.protocol);
}

- (void)setProtocol:(NSString*)protocol {
  _dataChannelInit.protocol = StdStringFromNSString(protocol);
}

- (BOOL)isNegotiated {
  return _dataChannelInit.negotiated;
}

- (void)setIsNegotiated:(BOOL)isNegotiated {
  _dataChannelInit.negotiated = isNegotiated;
}

- (NSInteger)streamId {
  return _dataChannelInit.id;
}

- (void)setStreamId:(NSInteger)streamId {
  _dataChannelInit.id = streamId;
}

@end

@implementation RTCDataChannelInit (Internal)

- (const webrtc::DataChannelInit*)dataChannelInit {
  return &_dataChannelInit;
}

@end

@implementation RTCDataBuffer {
  talk_base::scoped_ptr<webrtc::DataBuffer> _dataBuffer;
}

- (instancetype)initWithData:(NSData*)data isBinary:(BOOL)isBinary {
  NSAssert(data, @"data cannot be nil");
  if (self = [super init]) {
    talk_base::Buffer buffer([data bytes], [data length]);
    _dataBuffer.reset(new webrtc::DataBuffer(buffer, isBinary));
  }
  return self;
}

- (NSData*)data {
  return [NSData dataWithBytes:_dataBuffer->data.data()
                        length:_dataBuffer->data.length()];
}

- (BOOL)isBinary {
  return _dataBuffer->binary;
}

@end

@implementation RTCDataBuffer (Internal)

- (instancetype)initWithDataBuffer:(const webrtc::DataBuffer&)buffer {
  if (self = [super init]) {
    _dataBuffer.reset(new webrtc::DataBuffer(buffer));
  }
  return self;
}

- (const webrtc::DataBuffer*)dataBuffer {
  return _dataBuffer.get();
}

@end

@implementation RTCDataChannel {
  talk_base::scoped_refptr<webrtc::DataChannelInterface> _dataChannel;
  talk_base::scoped_ptr<webrtc::RTCDataChannelObserver> _observer;
  BOOL _isObserverRegistered;
}

- (NSString*)label {
  return NSStringFromStdString(_dataChannel->label());
}

- (BOOL)isReliable {
  return _dataChannel->reliable();
}

- (BOOL)isOrdered {
  return _dataChannel->ordered();
}

- (NSUInteger)maxRetransmitTimeMs {
  return _dataChannel->maxRetransmitTime();
}

- (NSUInteger)maxRetransmits {
  return _dataChannel->maxRetransmits();
}

- (NSString*)protocol {
  return NSStringFromStdString(_dataChannel->protocol());
}

- (BOOL)isNegotiated {
  return _dataChannel->negotiated();
}

- (NSInteger)streamId {
  return _dataChannel->id();
}

- (RTCDataChannelState)state {
  switch (_dataChannel->state()) {
    case webrtc::DataChannelInterface::DataState::kConnecting:
      return kRTCDataChannelStateConnecting;
    case webrtc::DataChannelInterface::DataState::kOpen:
      return kRTCDataChannelStateOpen;
    case webrtc::DataChannelInterface::DataState::kClosing:
      return kRTCDataChannelStateClosing;
    case webrtc::DataChannelInterface::DataState::kClosed:
      return kRTCDataChannelStateClosed;
  }
}

- (NSUInteger)bufferedAmount {
  return _dataChannel->buffered_amount();
}

- (void)setDelegate:(id<RTCDataChannelDelegate>)delegate {
  if (_delegate == delegate) {
    return;
  }
  if (_isObserverRegistered) {
    _dataChannel->UnregisterObserver();
    _isObserverRegistered = NO;
  }
  _delegate = delegate;
  if (_delegate) {
    _dataChannel->RegisterObserver(_observer.get());
    _isObserverRegistered = YES;
  }
}

- (void)close {
  _dataChannel->Close();
}

- (BOOL)sendData:(RTCDataBuffer*)data {
  return _dataChannel->Send(*data.dataBuffer);
}

@end

@implementation RTCDataChannel (Internal)

- (instancetype)initWithDataChannel:
                    (talk_base::scoped_refptr<webrtc::DataChannelInterface>)
                dataChannel {
  NSAssert(dataChannel != NULL, @"dataChannel cannot be NULL");
  if (self = [super init]) {
    _dataChannel = dataChannel;
    _observer.reset(new webrtc::RTCDataChannelObserver(self));
  }
  return self;
}

- (talk_base::scoped_refptr<webrtc::DataChannelInterface>)dataChannel {
  return _dataChannel;
}

@end
