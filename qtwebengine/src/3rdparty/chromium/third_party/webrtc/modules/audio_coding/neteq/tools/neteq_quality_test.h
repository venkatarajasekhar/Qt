/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_NETEQ_QUALITY_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_NETEQ_QUALITY_TEST_H_

#include <string>
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/audio_coding/neteq/tools/input_audio_file.h"
#include "webrtc/modules/audio_coding/neteq/tools/rtp_generator.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class NetEqQualityTest : public ::testing::Test {
 protected:
  NetEqQualityTest(int block_duration_ms,
                   int in_sampling_khz,
                   int out_sampling_khz,
                   enum NetEqDecoder decoder_type,
                   int channels,
                   double drift_factor,
                   std::string in_filename,
                   std::string out_filename);
  virtual void SetUp() OVERRIDE;
  virtual void TearDown() OVERRIDE;

  // EncodeBlock(...) does the following:
  // 1. encodes a block of audio, saved in |in_data| and has a length of
  // |block_size_samples| (samples per channel),
  // 2. save the bit stream to |payload| of |max_bytes| bytes in size,
  // 3. returns the length of the payload (in bytes),
  virtual int EncodeBlock(int16_t* in_data, int block_size_samples,
                          uint8_t* payload, int max_bytes) = 0;

  // PacketLoss(...) determines weather a packet sent at an indicated time gets
  // lost or not.
  virtual bool PacketLost(int packet_input_time_ms) { return false; }

  // DecodeBlock() decodes a block of audio using the payload stored in
  // |payload_| with the length of |payload_size_bytes_| (bytes). The decoded
  // audio is to be stored in |out_data_|.
  int DecodeBlock();

  // Transmit() uses |rtp_generator_| to generate a packet and passes it to
  // |neteq_|.
  int Transmit();

  // Simulate(...) runs encoding / transmitting / decoding up to |end_time_ms|
  // (miliseconds), the resulted audio is stored in the file with the name of
  // |out_filename_|.
  void Simulate(int end_time_ms);

 private:
  int decoded_time_ms_;
  int decodable_time_ms_;
  double drift_factor_;
  const int block_duration_ms_;
  const int in_sampling_khz_;
  const int out_sampling_khz_;
  const enum NetEqDecoder decoder_type_;
  const int channels_;
  const std::string in_filename_;
  const std::string out_filename_;

  // Number of samples per channel in a frame.
  const int in_size_samples_;

  // Expected output number of samples per channel in a frame.
  const int out_size_samples_;

  int payload_size_bytes_;
  int max_payload_bytes_;

  scoped_ptr<InputAudioFile> in_file_;
  FILE* out_file_;

  scoped_ptr<RtpGenerator> rtp_generator_;
  scoped_ptr<NetEq> neteq_;

  scoped_ptr<int16_t[]> in_data_;
  scoped_ptr<uint8_t[]> payload_;
  scoped_ptr<int16_t[]> out_data_;
  WebRtcRTPHeader rtp_header_;
};

}  // namespace test
}  // namespace webrtc

#endif  // WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_NETEQ_QUALITY_TEST_H_
