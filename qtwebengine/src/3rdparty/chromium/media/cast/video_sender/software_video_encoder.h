// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_VIDEO_SENDER_SOFTWARE_VIDEO_ENCODER_H_
#define MEDIA_CAST_VIDEO_SENDER_SOFTWARE_VIDEO_ENCODER_H_

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"

namespace media {
class VideoFrame;
}

namespace media {
namespace cast {
namespace transport {
struct EncodedFrame;
}  // namespace transport

class SoftwareVideoEncoder {
 public:
  virtual ~SoftwareVideoEncoder() {}

  // Initialize the encoder before Encode() can be called. This method
  // must be called on the thread that Encode() is called.
  virtual void Initialize() = 0;

  // Encode a raw image (as a part of a video stream).
  virtual bool Encode(const scoped_refptr<media::VideoFrame>& video_frame,
                      transport::EncodedFrame* encoded_image) = 0;

  // Update the encoder with a new target bit rate.
  virtual void UpdateRates(uint32 new_bitrate) = 0;

  // Set the next frame to be a key frame.
  virtual void GenerateKeyFrame() = 0;

  // Set the last frame to reference.
  virtual void LatestFrameIdToReference(uint32 frame_id) = 0;
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_VIDEO_SENDER_SOFTWARE_VIDEO_ENCODER_H_
