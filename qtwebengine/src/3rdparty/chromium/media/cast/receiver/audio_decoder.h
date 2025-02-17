// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_RECEIVER_AUDIO_DECODER_H_
#define MEDIA_CAST_RECEIVER_AUDIO_DECODER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "media/base/audio_bus.h"
#include "media/cast/cast_config.h"
#include "media/cast/cast_environment.h"
#include "media/cast/transport/cast_transport_config.h"

namespace media {
namespace cast {

class AudioDecoder {
 public:
  // Callback passed to DecodeFrame, to deliver decoded audio data from the
  // decoder.  The number of samples in |audio_bus| may vary, and |audio_bus|
  // can be NULL when errors occur.  |is_continuous| is normally true, but will
  // be false if the decoder has detected a frame skip since the last decode
  // operation; and the client should take steps to smooth audio discontinuities
  // in this case.
  typedef base::Callback<void(scoped_ptr<AudioBus> audio_bus,
                              bool is_continuous)> DecodeFrameCallback;

  AudioDecoder(const scoped_refptr<CastEnvironment>& cast_environment,
               int channels,
               int sampling_rate,
               transport::AudioCodec codec);
  virtual ~AudioDecoder();

  // Returns STATUS_AUDIO_INITIALIZED if the decoder was successfully
  // constructed from the given FrameReceiverConfig.  If this method returns any
  // other value, calls to DecodeFrame() will not succeed.
  CastInitializationStatus InitializationResult() const;

  // Decode the payload in |encoded_frame| asynchronously.  |callback| will be
  // invoked on the CastEnvironment::MAIN thread with the result.
  //
  // In the normal case, |encoded_frame->frame_id| will be
  // monotonically-increasing by 1 for each successive call to this method.
  // When it is not, the decoder will assume one or more frames have been
  // dropped (e.g., due to packet loss), and will perform recovery actions.
  void DecodeFrame(scoped_ptr<transport::EncodedFrame> encoded_frame,
                   const DecodeFrameCallback& callback);

 private:
  class ImplBase;
  class OpusImpl;
  class Pcm16Impl;

  const scoped_refptr<CastEnvironment> cast_environment_;
  scoped_refptr<ImplBase> impl_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoder);
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_RECEIVER_AUDIO_DECODER_H_
