/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <string.h>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/neteq/tools/audio_loop.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/system_wrappers/interface/thread_annotations.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace webrtc {

const int kSampleRateHz = 16000;
const int kNumSamples10ms = kSampleRateHz / 100;
const int kFrameSizeMs = 10;  // Multiple of 10.
const int kFrameSizeSamples = kFrameSizeMs / 10 * kNumSamples10ms;
const int kPayloadSizeBytes = kFrameSizeSamples * sizeof(int16_t);
const uint8_t kPayloadType = 111;

class RtpUtility {
 public:
  RtpUtility(int samples_per_packet, uint8_t payload_type)
      : samples_per_packet_(samples_per_packet), payload_type_(payload_type) {}

  virtual ~RtpUtility() {}

  void Populate(WebRtcRTPHeader* rtp_header) {
    rtp_header->header.sequenceNumber = 0xABCD;
    rtp_header->header.timestamp = 0xABCDEF01;
    rtp_header->header.payloadType = payload_type_;
    rtp_header->header.markerBit = false;
    rtp_header->header.ssrc = 0x1234;
    rtp_header->header.numCSRCs = 0;
    rtp_header->frameType = kAudioFrameSpeech;

    rtp_header->header.payload_type_frequency = kSampleRateHz;
    rtp_header->type.Audio.channel = 1;
    rtp_header->type.Audio.isCNG = false;
  }

  void Forward(WebRtcRTPHeader* rtp_header) {
    ++rtp_header->header.sequenceNumber;
    rtp_header->header.timestamp += samples_per_packet_;
  }

 private:
  int samples_per_packet_;
  uint8_t payload_type_;
};

class PacketizationCallbackStub : public AudioPacketizationCallback {
 public:
  PacketizationCallbackStub()
      : num_calls_(0),
        crit_sect_(CriticalSectionWrapper::CreateCriticalSection()) {}

  virtual int32_t SendData(
      FrameType frame_type,
      uint8_t payload_type,
      uint32_t timestamp,
      const uint8_t* payload_data,
      uint16_t payload_len_bytes,
      const RTPFragmentationHeader* fragmentation) OVERRIDE {
    CriticalSectionScoped lock(crit_sect_.get());
    ++num_calls_;
    last_payload_vec_.assign(payload_data, payload_data + payload_len_bytes);
    return 0;
  }

  int num_calls() const {
    CriticalSectionScoped lock(crit_sect_.get());
    return num_calls_;
  }

  int last_payload_len_bytes() const {
    CriticalSectionScoped lock(crit_sect_.get());
    return last_payload_vec_.size();
  }

  void SwapBuffers(std::vector<uint8_t>* payload) {
    CriticalSectionScoped lock(crit_sect_.get());
    last_payload_vec_.swap(*payload);
  }

 private:
  int num_calls_ GUARDED_BY(crit_sect_);
  std::vector<uint8_t> last_payload_vec_ GUARDED_BY(crit_sect_);
  const scoped_ptr<CriticalSectionWrapper> crit_sect_;
};

class AudioCodingModuleTest : public ::testing::Test {
 protected:
  AudioCodingModuleTest()
      : id_(1),
        rtp_utility_(new RtpUtility(kFrameSizeSamples, kPayloadType)),
        clock_(Clock::GetRealTimeClock()) {}

  ~AudioCodingModuleTest() {}

  void TearDown() {}

  void SetUp() {
    acm_.reset(AudioCodingModule::Create(id_, clock_));

    RegisterCodec();

    rtp_utility_->Populate(&rtp_header_);

    input_frame_.sample_rate_hz_ = kSampleRateHz;
    input_frame_.num_channels_ = 1;
    input_frame_.samples_per_channel_ = kSampleRateHz * 10 / 1000;  // 10 ms.
    COMPILE_ASSERT(kSampleRateHz * 10 / 1000 <= AudioFrame::kMaxDataSizeSamples,
                   audio_frame_too_small);
    memset(input_frame_.data_,
           0,
           input_frame_.samples_per_channel_ * sizeof(input_frame_.data_[0]));

    ASSERT_EQ(0, acm_->RegisterTransportCallback(&packet_cb_));
  }

  virtual void RegisterCodec() {
    AudioCodingModule::Codec("L16", &codec_, kSampleRateHz, 1);
    codec_.pltype = kPayloadType;

    // Register L16 codec in ACM.
    ASSERT_EQ(0, acm_->RegisterReceiveCodec(codec_));
    ASSERT_EQ(0, acm_->RegisterSendCodec(codec_));
  }

  virtual void InsertPacketAndPullAudio() {
    InsertPacket();
    PullAudio();
  }

  virtual void InsertPacket() {
    const uint8_t kPayload[kPayloadSizeBytes] = {0};
    ASSERT_EQ(0,
              acm_->IncomingPacket(kPayload, kPayloadSizeBytes, rtp_header_));
    rtp_utility_->Forward(&rtp_header_);
  }

  virtual void PullAudio() {
    AudioFrame audio_frame;
    ASSERT_EQ(0, acm_->PlayoutData10Ms(-1, &audio_frame));
  }

  virtual void InsertAudio() {
    ASSERT_EQ(0, acm_->Add10MsData(input_frame_));
    input_frame_.timestamp_ += kNumSamples10ms;
  }

  virtual void Encode() {
    int32_t encoded_bytes = acm_->Process();
    // Expect to get one packet with two bytes per sample, or no packet at all,
    // depending on how many 10 ms blocks go into |codec_.pacsize|.
    EXPECT_TRUE(encoded_bytes == 2 * codec_.pacsize || encoded_bytes == 0);
  }

  const int id_;
  scoped_ptr<RtpUtility> rtp_utility_;
  scoped_ptr<AudioCodingModule> acm_;
  PacketizationCallbackStub packet_cb_;
  WebRtcRTPHeader rtp_header_;
  AudioFrame input_frame_;
  CodecInst codec_;
  Clock* clock_;
};

// Check if the statistics are initialized correctly. Before any call to ACM
// all fields have to be zero.
TEST_F(AudioCodingModuleTest, DISABLED_ON_ANDROID(InitializedToZero)) {
  AudioDecodingCallStats stats;
  acm_->GetDecodingCallStatistics(&stats);
  EXPECT_EQ(0, stats.calls_to_neteq);
  EXPECT_EQ(0, stats.calls_to_silence_generator);
  EXPECT_EQ(0, stats.decoded_normal);
  EXPECT_EQ(0, stats.decoded_cng);
  EXPECT_EQ(0, stats.decoded_plc);
  EXPECT_EQ(0, stats.decoded_plc_cng);
}

// Apply an initial playout delay. Calls to AudioCodingModule::PlayoutData10ms()
// should result in generating silence, check the associated field.
TEST_F(AudioCodingModuleTest, DISABLED_ON_ANDROID(SilenceGeneratorCalled)) {
  AudioDecodingCallStats stats;
  const int kInitialDelay = 100;

  acm_->SetInitialPlayoutDelay(kInitialDelay);

  int num_calls = 0;
  for (int time_ms = 0; time_ms < kInitialDelay;
       time_ms += kFrameSizeMs, ++num_calls) {
    InsertPacketAndPullAudio();
  }
  acm_->GetDecodingCallStatistics(&stats);
  EXPECT_EQ(0, stats.calls_to_neteq);
  EXPECT_EQ(num_calls, stats.calls_to_silence_generator);
  EXPECT_EQ(0, stats.decoded_normal);
  EXPECT_EQ(0, stats.decoded_cng);
  EXPECT_EQ(0, stats.decoded_plc);
  EXPECT_EQ(0, stats.decoded_plc_cng);
}

// Insert some packets and pull audio. Check statistics are valid. Then,
// simulate packet loss and check if PLC and PLC-to-CNG statistics are
// correctly updated.
TEST_F(AudioCodingModuleTest, DISABLED_ON_ANDROID(NetEqCalls)) {
  AudioDecodingCallStats stats;
  const int kNumNormalCalls = 10;

  for (int num_calls = 0; num_calls < kNumNormalCalls; ++num_calls) {
    InsertPacketAndPullAudio();
  }
  acm_->GetDecodingCallStatistics(&stats);
  EXPECT_EQ(kNumNormalCalls, stats.calls_to_neteq);
  EXPECT_EQ(0, stats.calls_to_silence_generator);
  EXPECT_EQ(kNumNormalCalls, stats.decoded_normal);
  EXPECT_EQ(0, stats.decoded_cng);
  EXPECT_EQ(0, stats.decoded_plc);
  EXPECT_EQ(0, stats.decoded_plc_cng);

  const int kNumPlc = 3;
  const int kNumPlcCng = 5;

  // Simulate packet-loss. NetEq first performs PLC then PLC fades to CNG.
  for (int n = 0; n < kNumPlc + kNumPlcCng; ++n) {
    PullAudio();
  }
  acm_->GetDecodingCallStatistics(&stats);
  EXPECT_EQ(kNumNormalCalls + kNumPlc + kNumPlcCng, stats.calls_to_neteq);
  EXPECT_EQ(0, stats.calls_to_silence_generator);
  EXPECT_EQ(kNumNormalCalls, stats.decoded_normal);
  EXPECT_EQ(0, stats.decoded_cng);
  EXPECT_EQ(kNumPlc, stats.decoded_plc);
  EXPECT_EQ(kNumPlcCng, stats.decoded_plc_cng);
}

TEST_F(AudioCodingModuleTest, VerifyOutputFrame) {
  AudioFrame audio_frame;
  const int kSampleRateHz = 32000;
  EXPECT_EQ(0, acm_->PlayoutData10Ms(kSampleRateHz, &audio_frame));
  EXPECT_EQ(id_, audio_frame.id_);
  EXPECT_EQ(0u, audio_frame.timestamp_);
  EXPECT_GT(audio_frame.num_channels_, 0);
  EXPECT_EQ(kSampleRateHz / 100, audio_frame.samples_per_channel_);
  EXPECT_EQ(kSampleRateHz, audio_frame.sample_rate_hz_);
}

TEST_F(AudioCodingModuleTest, FailOnZeroDesiredFrequency) {
  AudioFrame audio_frame;
  EXPECT_EQ(-1, acm_->PlayoutData10Ms(0, &audio_frame));
}

// A multi-threaded test for ACM. This base class is using the PCM16b 16 kHz
// codec, while the derive class AcmIsacMtTest is using iSAC.
class AudioCodingModuleMtTest : public AudioCodingModuleTest {
 protected:
  static const int kNumPackets = 500;
  static const int kNumPullCalls = 500;

  AudioCodingModuleMtTest()
      : AudioCodingModuleTest(),
        send_thread_(ThreadWrapper::CreateThread(CbSendThread,
                                                 this,
                                                 kRealtimePriority,
                                                 "send")),
        insert_packet_thread_(ThreadWrapper::CreateThread(CbInsertPacketThread,
                                                          this,
                                                          kRealtimePriority,
                                                          "insert_packet")),
        pull_audio_thread_(ThreadWrapper::CreateThread(CbPullAudioThread,
                                                       this,
                                                       kRealtimePriority,
                                                       "pull_audio")),
        test_complete_(EventWrapper::Create()),
        send_count_(0),
        insert_packet_count_(0),
        pull_audio_count_(0),
        crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
        next_insert_packet_time_ms_(0),
        fake_clock_(new SimulatedClock(0)) {
    clock_ = fake_clock_.get();
  }

  void SetUp() {
    AudioCodingModuleTest::SetUp();
    StartThreads();
  }

  void StartThreads() {
    unsigned int thread_id = 0;
    ASSERT_TRUE(send_thread_->Start(thread_id));
    ASSERT_TRUE(insert_packet_thread_->Start(thread_id));
    ASSERT_TRUE(pull_audio_thread_->Start(thread_id));
  }

  void TearDown() {
    AudioCodingModuleTest::TearDown();
    pull_audio_thread_->Stop();
    send_thread_->Stop();
    insert_packet_thread_->Stop();
  }

  EventTypeWrapper RunTest() {
    return test_complete_->Wait(10 * 60 * 1000);  // 10 minutes' timeout.
  }

  virtual bool TestDone() {
    if (packet_cb_.num_calls() > kNumPackets) {
      CriticalSectionScoped lock(crit_sect_.get());
      if (pull_audio_count_ > kNumPullCalls) {
        // Both conditions for completion are met. End the test.
        return true;
      }
    }
    return false;
  }

  static bool CbSendThread(void* context) {
    return reinterpret_cast<AudioCodingModuleMtTest*>(context)->CbSendImpl();
  }

  // The send thread doesn't have to care about the current simulated time,
  // since only the AcmReceiver is using the clock.
  bool CbSendImpl() {
    SleepMs(1);
    if (HasFatalFailure()) {
      // End the test early if a fatal failure (ASSERT_*) has occurred.
      test_complete_->Set();
    }
    ++send_count_;
    InsertAudio();
    Encode();
    if (TestDone()) {
      test_complete_->Set();
    }
    return true;
  }

  static bool CbInsertPacketThread(void* context) {
    return reinterpret_cast<AudioCodingModuleMtTest*>(context)
        ->CbInsertPacketImpl();
  }

  bool CbInsertPacketImpl() {
    SleepMs(1);
    {
      CriticalSectionScoped lock(crit_sect_.get());
      if (clock_->TimeInMilliseconds() < next_insert_packet_time_ms_) {
        return true;
      }
      next_insert_packet_time_ms_ += 10;
    }
    // Now we're not holding the crit sect when calling ACM.
    ++insert_packet_count_;
    InsertPacket();
    return true;
  }

  static bool CbPullAudioThread(void* context) {
    return reinterpret_cast<AudioCodingModuleMtTest*>(context)
        ->CbPullAudioImpl();
  }

  bool CbPullAudioImpl() {
    SleepMs(1);
    {
      CriticalSectionScoped lock(crit_sect_.get());
      // Don't let the insert thread fall behind.
      if (next_insert_packet_time_ms_ < clock_->TimeInMilliseconds()) {
        return true;
      }
      ++pull_audio_count_;
    }
    // Now we're not holding the crit sect when calling ACM.
    PullAudio();
    fake_clock_->AdvanceTimeMilliseconds(10);
    return true;
  }

  scoped_ptr<ThreadWrapper> send_thread_;
  scoped_ptr<ThreadWrapper> insert_packet_thread_;
  scoped_ptr<ThreadWrapper> pull_audio_thread_;
  const scoped_ptr<EventWrapper> test_complete_;
  int send_count_;
  int insert_packet_count_;
  int pull_audio_count_ GUARDED_BY(crit_sect_);
  const scoped_ptr<CriticalSectionWrapper> crit_sect_;
  int64_t next_insert_packet_time_ms_ GUARDED_BY(crit_sect_);
  scoped_ptr<SimulatedClock> fake_clock_;
};

TEST_F(AudioCodingModuleMtTest, DoTest) {
  EXPECT_EQ(kEventSignaled, RunTest());
}

// This is a multi-threaded ACM test using iSAC. The test encodes audio
// from a PCM file. The most recent encoded frame is used as input to the
// receiving part. Depending on timing, it may happen that the same RTP packet
// is inserted into the receiver multiple times, but this is a valid use-case,
// and simplifies the test code a lot.
class AcmIsacMtTest : public AudioCodingModuleMtTest {
 protected:
  static const int kNumPackets = 500;
  static const int kNumPullCalls = 500;

  AcmIsacMtTest()
      : AudioCodingModuleMtTest(),
        last_packet_number_(0) {}

  ~AcmIsacMtTest() {}

  void SetUp() {
    AudioCodingModuleTest::SetUp();

    // Set up input audio source to read from specified file, loop after 5
    // seconds, and deliver blocks of 10 ms.
    const std::string input_file_name =
        webrtc::test::ResourcePath("audio_coding/speech_mono_16kHz", "pcm");
    audio_loop_.Init(input_file_name, 5 * kSampleRateHz, kNumSamples10ms);

    // Generate one packet to have something to insert.
    int loop_counter = 0;
    while (packet_cb_.last_payload_len_bytes() == 0) {
      InsertAudio();
      Encode();
      ASSERT_LT(loop_counter++, 10);
    }
    // Set |last_packet_number_| to one less that |num_calls| so that the packet
    // will be fetched in the next InsertPacket() call.
    last_packet_number_ = packet_cb_.num_calls() - 1;

    StartThreads();
  }

  virtual void RegisterCodec() {
    COMPILE_ASSERT(kSampleRateHz == 16000, test_designed_for_isac_16khz);
    AudioCodingModule::Codec("ISAC", &codec_, kSampleRateHz, 1);
    codec_.pltype = kPayloadType;

    // Register iSAC codec in ACM, effectively unregistering the PCM16B codec
    // registered in AudioCodingModuleTest::SetUp();
    ASSERT_EQ(0, acm_->RegisterReceiveCodec(codec_));
    ASSERT_EQ(0, acm_->RegisterSendCodec(codec_));
  }

  void InsertPacket() {
    int num_calls = packet_cb_.num_calls();  // Store locally for thread safety.
    if (num_calls > last_packet_number_) {
      // Get the new payload out from the callback handler.
      // Note that since we swap buffers here instead of directly inserting
      // a pointer to the data in |packet_cb_|, we avoid locking the callback
      // for the duration of the IncomingPacket() call.
      packet_cb_.SwapBuffers(&last_payload_vec_);
      ASSERT_GT(last_payload_vec_.size(), 0u);
      rtp_utility_->Forward(&rtp_header_);
      last_packet_number_ = num_calls;
    }
    ASSERT_GT(last_payload_vec_.size(), 0u);
    ASSERT_EQ(
        0,
        acm_->IncomingPacket(
            &last_payload_vec_[0], last_payload_vec_.size(), rtp_header_));
  }

  void InsertAudio() {
    memcpy(input_frame_.data_, audio_loop_.GetNextBlock(), kNumSamples10ms);
    AudioCodingModuleTest::InsertAudio();
  }

  void Encode() { ASSERT_GE(acm_->Process(), 0); }

  // This method is the same as AudioCodingModuleMtTest::TestDone(), but here
  // it is using the constants defined in this class (i.e., shorter test run).
  virtual bool TestDone() {
    if (packet_cb_.num_calls() > kNumPackets) {
      CriticalSectionScoped lock(crit_sect_.get());
      if (pull_audio_count_ > kNumPullCalls) {
        // Both conditions for completion are met. End the test.
        return true;
      }
    }
    return false;
  }

  int last_packet_number_;
  std::vector<uint8_t> last_payload_vec_;
  test::AudioLoop audio_loop_;
};

TEST_F(AcmIsacMtTest, DoTest) {
  EXPECT_EQ(kEventSignaled, RunTest());
}

}  // namespace webrtc
