// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/strings/string_number_conversions.h"
#include "media/audio/audio_parameters.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(AudioParameters, Constructor_Default) {
  AudioParameters::Format expected_format = AudioParameters::AUDIO_PCM_LINEAR;
  int expected_bits = 0;
  int expected_channels = 0;
  ChannelLayout expected_channel_layout = CHANNEL_LAYOUT_NONE;
  int expected_rate = 0;
  int expected_samples = 0;

  AudioParameters params;

  EXPECT_EQ(expected_format, params.format());
  EXPECT_EQ(expected_bits, params.bits_per_sample());
  EXPECT_EQ(expected_channels, params.channels());
  EXPECT_EQ(expected_channel_layout, params.channel_layout());
  EXPECT_EQ(expected_rate, params.sample_rate());
  EXPECT_EQ(expected_samples, params.frames_per_buffer());
}

TEST(AudioParameters, Constructor_ParameterValues) {
  AudioParameters::Format expected_format =
      AudioParameters::AUDIO_PCM_LOW_LATENCY;
  int expected_bits = 16;
  int expected_channels = 6;
  ChannelLayout expected_channel_layout = CHANNEL_LAYOUT_5_1;
  int expected_rate = 44100;
  int expected_samples = 880;

  AudioParameters params(expected_format, expected_channel_layout,
                         expected_rate, expected_bits, expected_samples);

  EXPECT_EQ(expected_format, params.format());
  EXPECT_EQ(expected_bits, params.bits_per_sample());
  EXPECT_EQ(expected_channels, params.channels());
  EXPECT_EQ(expected_channel_layout, params.channel_layout());
  EXPECT_EQ(expected_rate, params.sample_rate());
  EXPECT_EQ(expected_samples, params.frames_per_buffer());
}

TEST(AudioParameters, GetBytesPerBuffer) {
  EXPECT_EQ(100, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_MONO, 1000,  8, 100)
                                 .GetBytesPerBuffer());
  EXPECT_EQ(200, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_MONO, 1000,  16, 100)
                                 .GetBytesPerBuffer());
  EXPECT_EQ(200, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_STEREO, 1000,  8, 100)
                                 .GetBytesPerBuffer());
  EXPECT_EQ(200, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_MONO, 1000,  8, 200)
                                 .GetBytesPerBuffer());
  EXPECT_EQ(800, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_STEREO, 1000,  16, 200)
                                 .GetBytesPerBuffer());
  EXPECT_EQ(300, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC,
                                 1000, 8, 100)
                                 .GetBytesPerBuffer());
}

TEST(AudioParameters, GetBytesPerSecond) {
  EXPECT_EQ(0, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                               CHANNEL_LAYOUT_NONE, 0, 0, 0)
                               .GetBytesPerSecond());
  EXPECT_EQ(0, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                               CHANNEL_LAYOUT_STEREO, 0, 0, 0)
                               .GetBytesPerSecond());
  EXPECT_EQ(0, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                               CHANNEL_LAYOUT_NONE, 100, 0, 0)
                               .GetBytesPerSecond());
  EXPECT_EQ(0, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                               CHANNEL_LAYOUT_NONE, 0, 8, 0)
                               .GetBytesPerSecond());
  EXPECT_EQ(200, AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                                 CHANNEL_LAYOUT_STEREO, 100, 8, 0)
                                 .GetBytesPerSecond());
}

TEST(AudioParameters, Compare) {
  AudioParameters values[] = {
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    1000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    1000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    1000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    1000, 16, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    2000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    2000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    2000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_MONO,
                    2000, 16, 200),

    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    1000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    1000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    1000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    1000, 16, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    2000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    2000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    2000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, CHANNEL_LAYOUT_STEREO,
                    2000, 16, 200),

    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000, 16, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000, 16, 200),

    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    1000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    1000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    1000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    1000, 16, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    2000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    2000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    2000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
                    2000, 16, 200),

    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 1000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 1000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 1000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 1000, 16, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 2000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 2000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 2000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO, 2000, 16, 200),

    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 1000, 16, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000,  8, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000,  8, 200),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000, 16, 100),
    AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                    CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC, 2000, 16, 200),
  };

  for (size_t i = 0; i < arraysize(values); ++i) {
    for (size_t j = 0; j < arraysize(values); ++j) {
      SCOPED_TRACE("i=" + base::IntToString(i) + " j=" + base::IntToString(j));
      EXPECT_EQ(i < j, values[i] < values[j]);
    }

    // Verify that a value is never less than itself.
    EXPECT_FALSE(values[i] < values[i]);
  }
}

}  // namespace media
