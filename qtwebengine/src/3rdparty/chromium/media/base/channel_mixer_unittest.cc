// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// MSVC++ requires this to be set before any other includes to get M_SQRT1_2.
#define _USE_MATH_DEFINES

#include <cmath>

#include "base/strings/stringprintf.h"
#include "media/audio/audio_parameters.h"
#include "media/base/audio_bus.h"
#include "media/base/channel_mixer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

// Number of frames to test with.
enum { kFrames = 16 };

// Test all possible layout conversions can be constructed and mixed.
TEST(ChannelMixerTest, ConstructAllPossibleLayouts) {
  for (ChannelLayout input_layout = CHANNEL_LAYOUT_MONO;
       input_layout <= CHANNEL_LAYOUT_MAX;
       input_layout = static_cast<ChannelLayout>(input_layout + 1)) {
    for (ChannelLayout output_layout = CHANNEL_LAYOUT_MONO;
         output_layout < CHANNEL_LAYOUT_STEREO_DOWNMIX;
         output_layout = static_cast<ChannelLayout>(output_layout + 1)) {
      // DISCRETE can't be tested here based on the current approach.
      // CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC is not mixable.
      if (input_layout == CHANNEL_LAYOUT_DISCRETE ||
          input_layout == CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC ||
          output_layout == CHANNEL_LAYOUT_DISCRETE ||
          output_layout == CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC) {
        continue;
      }

      SCOPED_TRACE(base::StringPrintf(
          "Input Layout: %d, Output Layout: %d", input_layout, output_layout));
      ChannelMixer mixer(input_layout, output_layout);
      scoped_ptr<AudioBus> input_bus = AudioBus::Create(
          ChannelLayoutToChannelCount(input_layout), kFrames);
      scoped_ptr<AudioBus> output_bus = AudioBus::Create(
          ChannelLayoutToChannelCount(output_layout), kFrames);
      for (int ch = 0; ch < input_bus->channels(); ++ch)
        std::fill(input_bus->channel(ch), input_bus->channel(ch) + kFrames, 1);

      mixer.Transform(input_bus.get(), output_bus.get());
    }
  }
}

struct ChannelMixerTestData {
  ChannelMixerTestData(ChannelLayout input_layout, ChannelLayout output_layout,
                       float* channel_values, int num_channel_values,
                       float scale)
      : input_layout(input_layout),
        output_layout(output_layout),
        channel_values(channel_values),
        num_channel_values(num_channel_values),
        scale(scale) {
    input_channels = ChannelLayoutToChannelCount(input_layout);
    output_channels = ChannelLayoutToChannelCount(output_layout);
  }

  ChannelMixerTestData(ChannelLayout input_layout, int input_channels,
                       ChannelLayout output_layout, int output_channels,
                       float* channel_values, int num_channel_values)
      : input_layout(input_layout),
        input_channels(input_channels),
        output_layout(output_layout),
        output_channels(output_channels),
        channel_values(channel_values),
        num_channel_values(num_channel_values),
        scale(1.0f) {
  }

  std::string DebugString() const {
    return base::StringPrintf(
        "Input Layout: %d, Output Layout %d, Scale: %f", input_layout,
        output_layout, scale);
  }

  ChannelLayout input_layout;
  int input_channels;
  ChannelLayout output_layout;
  int output_channels;
  float* channel_values;
  int num_channel_values;
  float scale;
};

std::ostream& operator<<(std::ostream& os, const ChannelMixerTestData& data) {
  return os << data.DebugString();
}

class ChannelMixerTest : public testing::TestWithParam<ChannelMixerTestData> {};

// Verify channels are mixed and scaled correctly.  The test only works if all
// output channels have the same value.
TEST_P(ChannelMixerTest, Mixing) {
  ChannelLayout input_layout = GetParam().input_layout;
  int input_channels = GetParam().input_channels;
  scoped_ptr<AudioBus> input_bus = AudioBus::Create(input_channels, kFrames);
  AudioParameters input_audio(AudioParameters::AUDIO_PCM_LINEAR,
                              input_layout,
                              input_layout == CHANNEL_LAYOUT_DISCRETE ?
                                  input_channels :
                                  ChannelLayoutToChannelCount(input_layout),
                              0,
                              AudioParameters::kAudioCDSampleRate, 16,
                              kFrames,
                              AudioParameters::NO_EFFECTS);

  ChannelLayout output_layout = GetParam().output_layout;
  int output_channels = GetParam().output_channels;
  scoped_ptr<AudioBus> output_bus = AudioBus::Create(output_channels, kFrames);
  AudioParameters output_audio(AudioParameters::AUDIO_PCM_LINEAR,
                               output_layout,
                               output_layout == CHANNEL_LAYOUT_DISCRETE ?
                                  output_channels :
                                  ChannelLayoutToChannelCount(output_layout),
                               0,
                               AudioParameters::kAudioCDSampleRate, 16,
                               kFrames,
                               AudioParameters::NO_EFFECTS);

  const float* channel_values = GetParam().channel_values;
  ASSERT_EQ(input_bus->channels(), GetParam().num_channel_values);

  float expected_value = 0;
  float scale = GetParam().scale;
  for (int ch = 0; ch < input_bus->channels(); ++ch) {
    std::fill(input_bus->channel(ch), input_bus->channel(ch) + kFrames,
              channel_values[ch]);
    expected_value += channel_values[ch] * scale;
  }

  ChannelMixer mixer(input_audio, output_audio);
  mixer.Transform(input_bus.get(), output_bus.get());

  // Validate the output channel
  if (input_layout != CHANNEL_LAYOUT_DISCRETE) {
    for (int ch = 0; ch < output_bus->channels(); ++ch) {
      for (int frame = 0; frame < output_bus->frames(); ++frame) {
        ASSERT_FLOAT_EQ(output_bus->channel(ch)[frame], expected_value);
      }
    }
  } else {
    // Processing discrete mixing. If there is a matching input channel,
    // then the output channel should be set. If no input channel,
    // output channel should be 0
    for (int ch = 0; ch < output_bus->channels(); ++ch) {
      expected_value = (ch < input_channels) ? channel_values[ch] : 0;
      for (int frame = 0; frame < output_bus->frames(); ++frame) {
        ASSERT_FLOAT_EQ(output_bus->channel(ch)[frame], expected_value);
      }
    }
  }
}

static float kStereoToMonoValues[] = { 0.5f, 0.75f };
static float kMonoToStereoValues[] = { 0.5f };
// Zero the center channel since it will be mixed at scale 1 vs M_SQRT1_2.
static float kFiveOneToMonoValues[] = { 0.1f, 0.2f, 0.0f, 0.4f, 0.5f, 0.6f };
static float kFiveDiscreteValues[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };

// Run through basic sanity tests for some common conversions.
INSTANTIATE_TEST_CASE_P(ChannelMixerTest, ChannelMixerTest, testing::Values(
    ChannelMixerTestData(CHANNEL_LAYOUT_STEREO, CHANNEL_LAYOUT_MONO,
                         kStereoToMonoValues, arraysize(kStereoToMonoValues),
                         0.5f),
    ChannelMixerTestData(CHANNEL_LAYOUT_MONO, CHANNEL_LAYOUT_STEREO,
                         kMonoToStereoValues, arraysize(kMonoToStereoValues),
                         1.0f),
    ChannelMixerTestData(CHANNEL_LAYOUT_5_1, CHANNEL_LAYOUT_MONO,
                         kFiveOneToMonoValues, arraysize(kFiveOneToMonoValues),
                         static_cast<float>(M_SQRT1_2)),
    ChannelMixerTestData(CHANNEL_LAYOUT_DISCRETE, 2,
                         CHANNEL_LAYOUT_DISCRETE, 2,
                         kStereoToMonoValues, arraysize(kStereoToMonoValues)),
    ChannelMixerTestData(CHANNEL_LAYOUT_DISCRETE, 2,
                         CHANNEL_LAYOUT_DISCRETE, 5,
                         kStereoToMonoValues, arraysize(kStereoToMonoValues)),
    ChannelMixerTestData(CHANNEL_LAYOUT_DISCRETE, 5,
                         CHANNEL_LAYOUT_DISCRETE, 2,
                         kFiveDiscreteValues, arraysize(kFiveDiscreteValues))
));

}  // namespace media
