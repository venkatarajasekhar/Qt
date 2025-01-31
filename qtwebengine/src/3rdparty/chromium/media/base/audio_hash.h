// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_AUDIO_HASH_H_
#define MEDIA_BASE_AUDIO_HASH_H_

#include <string>

#include "base/basictypes.h"
#include "base/strings/string_piece.h"
#include "media/base/media_export.h"

namespace media {

class AudioBus;

// Computes a running hash for a series of AudioBus objects.  The hash is the
// sum of each sample bucketed based on the frame index, channel number, and
// current hash count.  The hash was designed with two properties in mind:
//
//   1. Uniform error distribution across the input sample.
//   2. Resilience to error below a certain threshold.
//
// The first is achieved by using a simple summing approach and moving position
// weighting into the bucket choice.  The second is handled during conversion to
// string by rounding out values to only two decimal places.
//
// Using only two decimal places allows for roughly -40 dBFS of error.  For
// reference, SincResampler produces an RMS error of around -15 dBFS.  See
// http://en.wikipedia.org/wiki/DBFS and http://crbug.com/168204 for more info.
class MEDIA_EXPORT AudioHash {
 public:
  AudioHash();
  ~AudioHash();

  // Update current hash with the contents of the provided AudioBus.
  void Update(const AudioBus* audio_bus, int frames);

  // Return a string representation of the current hash.
  std::string ToString() const;

 private:
  // Storage for the audio hash.  The number of buckets controls the importance
  // of position in the hash.  A higher number reduces the chance of false
  // positives related to incorrect sample position.  Value chosen by dice roll.
  enum { kHashBuckets = 6 };
  float audio_hash_[kHashBuckets];

  // The total number of samples processed per channel.  Uses a uint32 instead
  // of size_t so overflows on 64-bit and 32-bit machines are equivalent.
  uint32 sample_count_;

  DISALLOW_COPY_AND_ASSIGN(AudioHash);
};

}  // namespace media

#endif  // MEDIA_BASE_AUDIO_HASH_H_
