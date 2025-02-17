/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "webrtc/modules/audio_coding/codecs/tools/audio_codec_speed_test.h"

using ::std::string;
using ::std::tr1::make_tuple;
using ::testing::ValuesIn;

namespace webrtc {

static const int kOpusBlockDurationMs = 20;
static const int kOpusInputSamplingKhz = 48;
static const int kOpustOutputSamplingKhz = 32;

class OpusSpeedTest : public AudioCodecSpeedTest {
 protected:
  OpusSpeedTest();
  virtual void SetUp() OVERRIDE;
  virtual void TearDown() OVERRIDE;
  virtual float EncodeABlock(int16_t* in_data, uint8_t* bit_stream,
                             int max_bytes, int* encoded_bytes);
  virtual float DecodeABlock(const uint8_t* bit_stream, int encoded_bytes,
                             int16_t* out_data);
  WebRtcOpusEncInst* opus_encoder_;
  WebRtcOpusDecInst* opus_decoder_;
};

OpusSpeedTest::OpusSpeedTest()
    : AudioCodecSpeedTest(kOpusBlockDurationMs,
                          kOpusInputSamplingKhz,
                          kOpustOutputSamplingKhz),
      opus_encoder_(NULL),
      opus_decoder_(NULL) {
}

void OpusSpeedTest::SetUp() {
  AudioCodecSpeedTest::SetUp();
  /* Create encoder memory. */
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_encoder_, channels_));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_decoder_, channels_));
  /* Set bitrate. */
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_encoder_, bit_rate_));
}

void OpusSpeedTest::TearDown() {
  AudioCodecSpeedTest::TearDown();
  /* Free memory. */
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_decoder_));
}

float OpusSpeedTest::EncodeABlock(int16_t* in_data, uint8_t* bit_stream,
                                  int max_bytes, int* encoded_bytes) {
  clock_t clocks = clock();
  int value = WebRtcOpus_Encode(opus_encoder_, in_data,
                                input_length_sample_, max_bytes,
                                bit_stream);
  clocks = clock() - clocks;
  EXPECT_GT(value, 0);
  *encoded_bytes = value;
  return 1000.0 * clocks / CLOCKS_PER_SEC;
}

float OpusSpeedTest::DecodeABlock(const uint8_t* bit_stream,
                                  int encoded_bytes, int16_t* out_data) {
  int value;
  int16_t audio_type;
  clock_t clocks = clock();
  value = WebRtcOpus_DecodeNew(opus_decoder_, bit_stream, encoded_bytes,
                               out_data, &audio_type);
  clocks = clock() - clocks;
  EXPECT_EQ(output_length_sample_, value);
  return 1000.0 * clocks / CLOCKS_PER_SEC;
}

#define ADD_TEST(complexity) \
TEST_P(OpusSpeedTest, OpusSetComplexityTest##complexity) { \
  /* Test audio length in second. */ \
  size_t kDurationSec = 400; \
  /* Set complexity. */ \
  printf("Setting complexity to %d ...\n", complexity); \
  EXPECT_EQ(0, WebRtcOpus_SetComplexity(opus_encoder_, complexity)); \
  EncodeDecode(kDurationSec); \
}

ADD_TEST(10);
ADD_TEST(9);
ADD_TEST(8);
ADD_TEST(7);
ADD_TEST(6);
ADD_TEST(5);
ADD_TEST(4);
ADD_TEST(3);
ADD_TEST(2);
ADD_TEST(1);
ADD_TEST(0);

// List all test cases: (channel, bit rat, filename, extension).
const coding_param param_set[] =
    {make_tuple(1, 64000, string("audio_coding/speech_mono_32_48kHz"),
                string("pcm"), true),
     make_tuple(1, 32000, string("audio_coding/speech_mono_32_48kHz"),
                string("pcm"), true),
     make_tuple(2, 64000, string("audio_coding/music_stereo_48kHz"),
                string("pcm"), true)};

INSTANTIATE_TEST_CASE_P(AllTest, OpusSpeedTest,
                        ValuesIn(param_set));

}  // namespace webrtc
