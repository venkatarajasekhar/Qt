// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/audio_discard_helper.h"

#include <algorithm>

#include "base/logging.h"
#include "media/base/audio_buffer.h"
#include "media/base/buffers.h"

namespace media {

static void WarnOnNonMonotonicTimestamps(base::TimeDelta last_timestamp,
                                         base::TimeDelta current_timestamp) {
  if (last_timestamp == kNoTimestamp() || last_timestamp < current_timestamp)
    return;

  const base::TimeDelta diff = current_timestamp - last_timestamp;
  DLOG(WARNING) << "Input timestamps are not monotonically increasing! "
                << " ts " << current_timestamp.InMicroseconds() << " us"
                << " diff " << diff.InMicroseconds() << " us";
}

AudioDiscardHelper::AudioDiscardHelper(int sample_rate, size_t decoder_delay)
    : sample_rate_(sample_rate),
      decoder_delay_(decoder_delay),
      timestamp_helper_(sample_rate_),
      discard_frames_(0),
      last_input_timestamp_(kNoTimestamp()),
      delayed_discard_(false) {
  DCHECK_GT(sample_rate_, 0);
}

AudioDiscardHelper::~AudioDiscardHelper() {
}

size_t AudioDiscardHelper::TimeDeltaToFrames(base::TimeDelta duration) const {
  DCHECK(duration >= base::TimeDelta());
  return duration.InSecondsF() * sample_rate_ + 0.5;
}

void AudioDiscardHelper::Reset(size_t initial_discard) {
  discard_frames_ = initial_discard;
  last_input_timestamp_ = kNoTimestamp();
  timestamp_helper_.SetBaseTimestamp(kNoTimestamp());
  delayed_discard_ = false;
  delayed_discard_padding_ = DecoderBuffer::DiscardPadding();
}

bool AudioDiscardHelper::ProcessBuffers(
    const scoped_refptr<DecoderBuffer>& encoded_buffer,
    const scoped_refptr<AudioBuffer>& decoded_buffer) {
  DCHECK(!encoded_buffer->end_of_stream());
  DCHECK(encoded_buffer->timestamp() != kNoTimestamp());

  // Issue a debug warning when we see non-monotonic timestamps.  Only a warning
  // to allow chained OGG playback.
  WarnOnNonMonotonicTimestamps(last_input_timestamp_,
                               encoded_buffer->timestamp());
  last_input_timestamp_ = encoded_buffer->timestamp();

  // If this is the first buffer seen, setup the timestamp helper.
  const bool first_buffer = !initialized();
  if (first_buffer) {
    // Clamp the base timestamp to zero.
    timestamp_helper_.SetBaseTimestamp(
        std::max(base::TimeDelta(), encoded_buffer->timestamp()));
  }
  DCHECK(initialized());

  if (!decoded_buffer) {
    // If there's a one buffer delay for decoding, we need to save it so it can
    // be processed with the next decoder buffer.
    if (first_buffer) {
      delayed_discard_ = true;
      delayed_discard_padding_ = encoded_buffer->discard_padding();
    }
    return false;
  }

  const size_t original_frame_count = decoded_buffer->frame_count();

  // If there's a one buffer delay for decoding, pick up the last encoded
  // buffer's discard padding for processing with the current decoded buffer.
  DecoderBuffer::DiscardPadding current_discard_padding =
      encoded_buffer->discard_padding();
  if (delayed_discard_) {
    // For simplicity disallow cases where decoder delay is present with delayed
    // discard (no codecs at present).  Doing so allows us to avoid complexity
    // around endpoint tracking when handling complete buffer discards.
    DCHECK_EQ(decoder_delay_, 0u);
    std::swap(current_discard_padding, delayed_discard_padding_);
  }

  if (discard_frames_ > 0) {
    const size_t decoded_frames = decoded_buffer->frame_count();
    const size_t frames_to_discard = std::min(discard_frames_, decoded_frames);
    discard_frames_ -= frames_to_discard;

    // If everything would be discarded, indicate a new buffer is required.
    if (frames_to_discard == decoded_frames) {
      // For simplicity disallow cases where a buffer with discard padding is
      // present.  Doing so allows us to avoid complexity around tracking
      // discards across buffers.
      DCHECK(current_discard_padding.first == base::TimeDelta());
      DCHECK(current_discard_padding.second == base::TimeDelta());
      return false;
    }

    decoded_buffer->TrimStart(frames_to_discard);
  }

  // Handle front discard padding.
  if (current_discard_padding.first > base::TimeDelta()) {
    const size_t decoded_frames = decoded_buffer->frame_count();

    // If a complete buffer discard is requested and there's no decoder delay,
    // just discard all remaining frames from this buffer.  With decoder delay
    // we have to estimate the correct number of frames to discard based on the
    // duration of the encoded buffer.
    const size_t start_frames_to_discard =
        current_discard_padding.first == kInfiniteDuration()
            ? (decoder_delay_ > 0
                   ? TimeDeltaToFrames(encoded_buffer->duration())
                   : decoded_frames)
            : TimeDeltaToFrames(current_discard_padding.first);

    // Regardless of the timestamp on the encoded buffer, the corresponding
    // decoded output will appear |decoder_delay_| frames later.
    size_t discard_start = decoder_delay_;
    if (decoder_delay_ > 0) {
      // If we have a |decoder_delay_| and have already discarded frames from
      // this buffer, the |discard_start| must be adjusted by the number of
      // frames already discarded.
      const size_t frames_discarded_so_far =
          original_frame_count - decoded_buffer->frame_count();
      CHECK_LE(frames_discarded_so_far, decoder_delay_);
      discard_start -= frames_discarded_so_far;
    }

    // For simplicity require the start of the discard to be within the current
    // buffer.  Doing so allows us avoid complexity around tracking discards
    // across buffers.
    CHECK_LT(discard_start, decoded_frames);

    const size_t frames_to_discard =
        std::min(start_frames_to_discard, decoded_frames - discard_start);

    // Carry over any frames which need to be discarded from the front of the
    // next buffer.
    DCHECK(!discard_frames_);
    discard_frames_ = start_frames_to_discard - frames_to_discard;

    // If everything would be discarded, indicate a new buffer is required.
    if (frames_to_discard == decoded_frames) {
      // The buffer should not have been marked with end discard if the front
      // discard removes everything.
      DCHECK(current_discard_padding.second == base::TimeDelta());
      return false;
    }

    decoded_buffer->TrimRange(discard_start, discard_start + frames_to_discard);
  } else {
    DCHECK(current_discard_padding.first == base::TimeDelta());
  }

  // Handle end discard padding.
  if (current_discard_padding.second > base::TimeDelta()) {
    // Limit end discarding to when there is no |decoder_delay_|, otherwise it's
    // non-trivial determining where to start discarding end frames.
    CHECK(!decoder_delay_);

    const size_t decoded_frames = decoded_buffer->frame_count();
    const size_t end_frames_to_discard =
        TimeDeltaToFrames(current_discard_padding.second);

    if (end_frames_to_discard > decoded_frames) {
      DLOG(ERROR) << "Encountered invalid discard padding value.";
      return false;
    }

    // If everything would be discarded, indicate a new buffer is required.
    if (end_frames_to_discard == decoded_frames)
      return false;

    decoded_buffer->TrimEnd(end_frames_to_discard);
  } else {
    DCHECK(current_discard_padding.second == base::TimeDelta());
  }

  // Assign timestamp to the buffer.
  decoded_buffer->set_timestamp(timestamp_helper_.GetTimestamp());
  timestamp_helper_.AddFrames(decoded_buffer->frame_count());
  return true;
}

}  // namespace media
