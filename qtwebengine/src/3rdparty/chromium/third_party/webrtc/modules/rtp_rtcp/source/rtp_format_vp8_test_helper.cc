/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8_test_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc {

namespace test {

RtpFormatVp8TestHelper::RtpFormatVp8TestHelper(const RTPVideoHeaderVP8* hdr)
    : payload_data_(NULL),
      buffer_(NULL),
      data_ptr_(NULL),
      fragmentation_(NULL),
      hdr_info_(hdr),
      payload_start_(0),
      payload_size_(0),
      buffer_size_(0),
      sloppy_partitioning_(false),
      inited_(false) {}

RtpFormatVp8TestHelper::~RtpFormatVp8TestHelper() {
  delete fragmentation_;
  delete [] payload_data_;
  delete [] buffer_;
}

bool RtpFormatVp8TestHelper::Init(const int* partition_sizes,
                                  int num_partitions) {
  if (inited_) return false;
  fragmentation_ = new RTPFragmentationHeader;
  fragmentation_->VerifyAndAllocateFragmentationHeader(num_partitions);
  payload_size_ = 0;
  // Calculate sum payload size.
  for (int p = 0; p < num_partitions; ++p) {
    payload_size_ += partition_sizes[p];
  }
  buffer_size_ = payload_size_ + 6;  // Add space for payload descriptor.
  payload_data_ = new uint8_t[payload_size_];
  buffer_ = new uint8_t[buffer_size_];
  int j = 0;
  // Loop through the partitions again.
  for (int p = 0; p < num_partitions; ++p) {
    fragmentation_->fragmentationLength[p] = partition_sizes[p];
    fragmentation_->fragmentationOffset[p] = j;
    for (int i = 0; i < partition_sizes[p]; ++i) {
      assert(j < payload_size_);
      payload_data_[j++] = p;  // Set the payload value to the partition index.
    }
  }
  data_ptr_ = payload_data_;
  inited_ = true;
  return true;
}

void RtpFormatVp8TestHelper::GetAllPacketsAndCheck(
    RtpFormatVp8* packetizer,
    const int* expected_sizes,
    const int* expected_part,
    const bool* expected_frag_start,
    int expected_num_packets) {
  ASSERT_TRUE(inited_);
  int send_bytes = 0;
  bool last = false;
  for (int i = 0; i < expected_num_packets; ++i) {
    std::ostringstream ss;
    ss << "Checking packet " << i;
    SCOPED_TRACE(ss.str());
    EXPECT_EQ(expected_part[i],
              packetizer->NextPacket(buffer_, &send_bytes, &last));
    CheckPacket(send_bytes, expected_sizes[i], last,
                expected_frag_start[i]);
  }
  EXPECT_TRUE(last);
}

// Payload descriptor
//       0 1 2 3 4 5 6 7
//      +-+-+-+-+-+-+-+-+
//      |X|R|N|S|PartID | (REQUIRED)
//      +-+-+-+-+-+-+-+-+
// X:   |I|L|T|K|  RSV  | (OPTIONAL)
//      +-+-+-+-+-+-+-+-+
// I:   |   PictureID   | (OPTIONAL)
//      +-+-+-+-+-+-+-+-+
// L:   |   TL0PICIDX   | (OPTIONAL)
//      +-+-+-+-+-+-+-+-+
// T/K: | TID | KEYIDX  | (OPTIONAL)
//      +-+-+-+-+-+-+-+-+

// First octet tests.
#define EXPECT_BIT_EQ(x, n, a) EXPECT_EQ((((x) >> (n)) & 0x1), a)

#define EXPECT_RSV_ZERO(x) EXPECT_EQ(((x) & 0xE0), 0)

#define EXPECT_BIT_X_EQ(x, a) EXPECT_BIT_EQ(x, 7, a)

#define EXPECT_BIT_N_EQ(x, a) EXPECT_BIT_EQ(x, 5, a)

#define EXPECT_BIT_S_EQ(x, a) EXPECT_BIT_EQ(x, 4, a)

#define EXPECT_PART_ID_EQ(x, a) EXPECT_EQ(((x) & 0x0F), a)

// Extension fields tests
#define EXPECT_BIT_I_EQ(x, a) EXPECT_BIT_EQ(x, 7, a)

#define EXPECT_BIT_L_EQ(x, a) EXPECT_BIT_EQ(x, 6, a)

#define EXPECT_BIT_T_EQ(x, a) EXPECT_BIT_EQ(x, 5, a)

#define EXPECT_BIT_K_EQ(x, a) EXPECT_BIT_EQ(x, 4, a)

#define EXPECT_TID_EQ(x, a) EXPECT_EQ((((x) & 0xC0) >> 6), a)

#define EXPECT_BIT_Y_EQ(x, a) EXPECT_BIT_EQ(x, 5, a)

#define EXPECT_KEYIDX_EQ(x, a) EXPECT_EQ(((x) & 0x1F), a)

void RtpFormatVp8TestHelper::CheckHeader(bool frag_start) {
  payload_start_ = 1;
  EXPECT_BIT_EQ(buffer_[0], 6, 0);  // Check reserved bit.

  if (hdr_info_->pictureId != kNoPictureId ||
      hdr_info_->temporalIdx != kNoTemporalIdx ||
      hdr_info_->tl0PicIdx != kNoTl0PicIdx ||
      hdr_info_->keyIdx != kNoKeyIdx) {
    EXPECT_BIT_X_EQ(buffer_[0], 1);
    ++payload_start_;
    CheckPictureID();
    CheckTl0PicIdx();
    CheckTIDAndKeyIdx();
  } else {
    EXPECT_BIT_X_EQ(buffer_[0], 0);
  }

  EXPECT_BIT_N_EQ(buffer_[0], hdr_info_->nonReference ? 1 : 0);
  EXPECT_BIT_S_EQ(buffer_[0], frag_start ? 1 : 0);

  // Check partition index.
  if (!sloppy_partitioning_) {
    // The test payload data is constructed such that the payload value is the
    // same as the partition index.
    EXPECT_EQ(buffer_[0] & 0x0F, buffer_[payload_start_]);
  } else {
    // Partition should be set to 0.
    EXPECT_EQ(buffer_[0] & 0x0F, 0);
  }
}

// Verify that the I bit and the PictureID field are both set in accordance
// with the information in hdr_info_->pictureId.
void RtpFormatVp8TestHelper::CheckPictureID() {
  if (hdr_info_->pictureId != kNoPictureId) {
    EXPECT_BIT_I_EQ(buffer_[1], 1);
    if (hdr_info_->pictureId > 0x7F) {
      EXPECT_BIT_EQ(buffer_[payload_start_], 7, 1);
      EXPECT_EQ(buffer_[payload_start_] & 0x7F,
                (hdr_info_->pictureId >> 8) & 0x7F);
      EXPECT_EQ(buffer_[payload_start_ + 1],
                hdr_info_->pictureId & 0xFF);
      payload_start_ += 2;
    } else {
      EXPECT_BIT_EQ(buffer_[payload_start_], 7, 0);
      EXPECT_EQ(buffer_[payload_start_] & 0x7F,
                (hdr_info_->pictureId) & 0x7F);
      payload_start_ += 1;
    }
  } else {
    EXPECT_BIT_I_EQ(buffer_[1], 0);
  }
}

// Verify that the L bit and the TL0PICIDX field are both set in accordance
// with the information in hdr_info_->tl0PicIdx.
void RtpFormatVp8TestHelper::CheckTl0PicIdx() {
  if (hdr_info_->tl0PicIdx != kNoTl0PicIdx) {
    EXPECT_BIT_L_EQ(buffer_[1], 1);
    EXPECT_EQ(buffer_[payload_start_], hdr_info_->tl0PicIdx);
    ++payload_start_;
  } else {
    EXPECT_BIT_L_EQ(buffer_[1], 0);
  }
}

// Verify that the T bit and the TL0PICIDX field, and the K bit and KEYIDX
// field are all set in accordance with the information in
// hdr_info_->temporalIdx and hdr_info_->keyIdx, respectively.
void RtpFormatVp8TestHelper::CheckTIDAndKeyIdx() {
  if (hdr_info_->temporalIdx == kNoTemporalIdx &&
      hdr_info_->keyIdx == kNoKeyIdx) {
    EXPECT_BIT_T_EQ(buffer_[1], 0);
    EXPECT_BIT_K_EQ(buffer_[1], 0);
    return;
  }
  if (hdr_info_->temporalIdx != kNoTemporalIdx) {
    EXPECT_BIT_T_EQ(buffer_[1], 1);
    EXPECT_TID_EQ(buffer_[payload_start_], hdr_info_->temporalIdx);
    EXPECT_BIT_Y_EQ(buffer_[payload_start_], hdr_info_->layerSync ? 1 : 0);
  } else {
    EXPECT_BIT_T_EQ(buffer_[1], 0);
    EXPECT_TID_EQ(buffer_[payload_start_], 0);
    EXPECT_BIT_Y_EQ(buffer_[payload_start_], 0);
  }
  if (hdr_info_->keyIdx != kNoKeyIdx) {
    EXPECT_BIT_K_EQ(buffer_[1], 1);
    EXPECT_KEYIDX_EQ(buffer_[payload_start_], hdr_info_->keyIdx);
  } else {
    EXPECT_BIT_K_EQ(buffer_[1], 0);
    EXPECT_KEYIDX_EQ(buffer_[payload_start_], 0);
  }
  ++payload_start_;
}

// Verify that the payload (i.e., after the headers) of the packet stored in
// buffer_ is identical to the expected (as found in data_ptr_).
void RtpFormatVp8TestHelper::CheckPayload(int payload_end) {
  for (int i = payload_start_; i < payload_end; ++i, ++data_ptr_)
    EXPECT_EQ(buffer_[i], *data_ptr_);
}

// Verify that the input variable "last" agrees with the position of data_ptr_.
// If data_ptr_ has advanced payload_size_ bytes from the start (payload_data_)
// we are at the end and last should be true. Otherwise, it should be false.
void RtpFormatVp8TestHelper::CheckLast(bool last) const {
  EXPECT_EQ(last, data_ptr_ == payload_data_ + payload_size_);
}

// Verify the contents of a packet. Check the length versus expected_bytes,
// the header, payload, and "last" flag.
void RtpFormatVp8TestHelper::CheckPacket(int send_bytes,
                                         int expect_bytes,
                                         bool last,
                                         bool frag_start) {
  EXPECT_EQ(expect_bytes, send_bytes);
  CheckHeader(frag_start);
  CheckPayload(send_bytes);
  CheckLast(last);
}

}  // namespace test

}  // namespace webrtc
