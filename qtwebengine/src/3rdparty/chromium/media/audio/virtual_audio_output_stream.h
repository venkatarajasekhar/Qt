// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_VIRTUAL_AUDIO_OUTPUT_STREAM_H_
#define MEDIA_AUDIO_VIRTUAL_AUDIO_OUTPUT_STREAM_H_

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_parameters.h"
#include "media/base/audio_converter.h"

namespace media {

class VirtualAudioInputStream;

// VirtualAudioOutputStream attaches to a VirtualAudioInputStream when Start()
// is called and is used as an audio source. VirtualAudioOutputStream also
// implements an interface so it can be used as an input to AudioConverter so
// that we can get audio frames that match the AudioParameters that
// VirtualAudioInputStream expects.
class MEDIA_EXPORT VirtualAudioOutputStream
    : public AudioOutputStream,
      public AudioConverter::InputCallback {
 public:
  // Callback invoked just after VirtualAudioOutputStream is closed.
  typedef base::Callback<void(VirtualAudioOutputStream* vaos)>
      AfterCloseCallback;

  // Construct an audio loopback pathway to the given |target| (not owned).
  // |target| must outlive this instance.
  VirtualAudioOutputStream(const AudioParameters& params,
                           VirtualAudioInputStream* target,
                           const AfterCloseCallback& after_close_cb);

  virtual ~VirtualAudioOutputStream();

  // AudioOutputStream:
  virtual bool Open() OVERRIDE;
  virtual void Start(AudioSourceCallback* callback) OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual void SetVolume(double volume) OVERRIDE;
  virtual void GetVolume(double* volume) OVERRIDE;
  virtual void Close() OVERRIDE;

 private:
  // AudioConverter::InputCallback:
  virtual double ProvideInput(AudioBus* audio_bus,
                              base::TimeDelta buffer_delay) OVERRIDE;

  const AudioParameters params_;
  // Pointer to the VirtualAudioInputStream to attach to when Start() is called.
  // This pointer should always be valid because VirtualAudioInputStream should
  // outlive this class.
  VirtualAudioInputStream* const target_input_stream_;

  AfterCloseCallback after_close_cb_;

  AudioSourceCallback* callback_;
  double volume_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(VirtualAudioOutputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_VIRTUAL_AUDIO_OUTPUT_STREAM_H_
