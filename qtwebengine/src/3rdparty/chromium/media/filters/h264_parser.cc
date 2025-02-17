// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/stl_util.h"

#include "media/filters/h264_parser.h"

namespace media {

bool H264SliceHeader::IsPSlice() const {
  return (slice_type % 5 == kPSlice);
}

bool H264SliceHeader::IsBSlice() const {
  return (slice_type % 5 == kBSlice);
}

bool H264SliceHeader::IsISlice() const {
  return (slice_type % 5 == kISlice);
}

bool H264SliceHeader::IsSPSlice() const {
  return (slice_type % 5 == kSPSlice);
}

bool H264SliceHeader::IsSISlice() const {
  return (slice_type % 5 == kSISlice);
}

H264NALU::H264NALU() {
  memset(this, 0, sizeof(*this));
}

H264SPS::H264SPS() {
  memset(this, 0, sizeof(*this));
}

H264PPS::H264PPS() {
  memset(this, 0, sizeof(*this));
}

H264SliceHeader::H264SliceHeader() {
  memset(this, 0, sizeof(*this));
}

H264SEIMessage::H264SEIMessage() {
  memset(this, 0, sizeof(*this));
}

#define READ_BITS_OR_RETURN(num_bits, out)                                 \
  do {                                                                     \
    int _out;                                                              \
    if (!br_.ReadBits(num_bits, &_out)) {                                  \
      DVLOG(1)                                                             \
          << "Error in stream: unexpected EOS while trying to read " #out; \
      return kInvalidStream;                                               \
    }                                                                      \
    *out = _out;                                                           \
  } while (0)

#define READ_BOOL_OR_RETURN(out)                                           \
  do {                                                                     \
    int _out;                                                              \
    if (!br_.ReadBits(1, &_out)) {                                         \
      DVLOG(1)                                                             \
          << "Error in stream: unexpected EOS while trying to read " #out; \
      return kInvalidStream;                                               \
    }                                                                      \
    *out = _out != 0;                                                      \
  } while (0)

#define READ_UE_OR_RETURN(out)                                                 \
  do {                                                                         \
    if (ReadUE(out) != kOk) {                                                  \
      DVLOG(1) << "Error in stream: invalid value while trying to read " #out; \
      return kInvalidStream;                                                   \
    }                                                                          \
  } while (0)

#define READ_SE_OR_RETURN(out)                                                 \
  do {                                                                         \
    if (ReadSE(out) != kOk) {                                                  \
      DVLOG(1) << "Error in stream: invalid value while trying to read " #out; \
      return kInvalidStream;                                                   \
    }                                                                          \
  } while (0)

#define IN_RANGE_OR_RETURN(val, min, max)                                   \
  do {                                                                      \
    if ((val) < (min) || (val) > (max)) {                                   \
      DVLOG(1) << "Error in stream: invalid value, expected " #val " to be" \
               << " in range [" << (min) << ":" << (max) << "]"             \
               << " found " << (val) << " instead";                         \
      return kInvalidStream;                                                \
    }                                                                       \
  } while (0)

#define TRUE_OR_RETURN(a)                                            \
  do {                                                               \
    if (!(a)) {                                                      \
      DVLOG(1) << "Error in stream: invalid value, expected " << #a; \
      return kInvalidStream;                                         \
    }                                                                \
  } while (0)

enum AspectRatioIdc {
  kExtendedSar = 255,
};

// ISO 14496 part 10
// VUI parameters: Table E-1 "Meaning of sample aspect ratio indicator"
static const int kTableSarWidth[] = {
  0, 1, 12, 10, 16, 40, 24, 20, 32, 80, 18, 15, 64, 160, 4, 3, 2
};
static const int kTableSarHeight[] = {
  0, 1, 11, 11, 11, 33, 11, 11, 11, 33, 11, 11, 33, 99, 3, 2, 1
};
COMPILE_ASSERT(arraysize(kTableSarWidth) == arraysize(kTableSarHeight),
               sar_tables_must_have_same_size);

H264Parser::H264Parser() {
  Reset();
}

H264Parser::~H264Parser() {
  STLDeleteValues(&active_SPSes_);
  STLDeleteValues(&active_PPSes_);
}

void H264Parser::Reset() {
  stream_ = NULL;
  bytes_left_ = 0;
}

void H264Parser::SetStream(const uint8* stream, off_t stream_size) {
  DCHECK(stream);
  DCHECK_GT(stream_size, 0);

  stream_ = stream;
  bytes_left_ = stream_size;
}

const H264PPS* H264Parser::GetPPS(int pps_id) {
  return active_PPSes_[pps_id];
}

const H264SPS* H264Parser::GetSPS(int sps_id) {
  return active_SPSes_[sps_id];
}

static inline bool IsStartCode(const uint8* data) {
  return data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01;
}

// static
bool H264Parser::FindStartCode(const uint8* data, off_t data_size,
                               off_t* offset, off_t* start_code_size) {
  DCHECK_GE(data_size, 0);
  off_t bytes_left = data_size;

  while (bytes_left >= 3) {
    if (IsStartCode(data)) {
      // Found three-byte start code, set pointer at its beginning.
      *offset = data_size - bytes_left;
      *start_code_size = 3;

      // If there is a zero byte before this start code,
      // then it's actually a four-byte start code, so backtrack one byte.
      if (*offset > 0 && *(data - 1) == 0x00) {
        --(*offset);
        ++(*start_code_size);
      }

      return true;
    }

    ++data;
    --bytes_left;
  }

  // End of data: offset is pointing to the first byte that was not considered
  // as a possible start of a start code.
  // Note: there is no security issue when receiving a negative |data_size|
  // since in this case, |bytes_left| is equal to |data_size| and thus
  // |*offset| is equal to 0 (valid offset).
  *offset = data_size - bytes_left;
  *start_code_size = 0;
  return false;
}

bool H264Parser::LocateNALU(off_t* nalu_size, off_t* start_code_size) {
  // Find the start code of next NALU.
  off_t nalu_start_off = 0;
  off_t annexb_start_code_size = 0;
  if (!FindStartCode(stream_, bytes_left_,
                     &nalu_start_off, &annexb_start_code_size)) {
    DVLOG(4) << "Could not find start code, end of stream?";
    return false;
  }

  // Move the stream to the beginning of the NALU (pointing at the start code).
  stream_ += nalu_start_off;
  bytes_left_ -= nalu_start_off;

  const uint8* nalu_data = stream_ + annexb_start_code_size;
  off_t max_nalu_data_size = bytes_left_ - annexb_start_code_size;
  if (max_nalu_data_size <= 0) {
    DVLOG(3) << "End of stream";
    return false;
  }

  // Find the start code of next NALU;
  // if successful, |nalu_size_without_start_code| is the number of bytes from
  // after previous start code to before this one;
  // if next start code is not found, it is still a valid NALU since there
  // are some bytes left after the first start code: all the remaining bytes
  // belong to the current NALU.
  off_t next_start_code_size = 0;
  off_t nalu_size_without_start_code = 0;
  if (!FindStartCode(nalu_data, max_nalu_data_size,
                     &nalu_size_without_start_code, &next_start_code_size)) {
    nalu_size_without_start_code = max_nalu_data_size;
  }
  *nalu_size = nalu_size_without_start_code + annexb_start_code_size;
  *start_code_size = annexb_start_code_size;
  return true;
}

H264Parser::Result H264Parser::ReadUE(int* val) {
  int num_bits = -1;
  int bit;
  int rest;

  // Count the number of contiguous zero bits.
  do {
    READ_BITS_OR_RETURN(1, &bit);
    num_bits++;
  } while (bit == 0);

  if (num_bits > 31)
    return kInvalidStream;

  // Calculate exp-Golomb code value of size num_bits.
  *val = (1 << num_bits) - 1;

  if (num_bits > 0) {
    READ_BITS_OR_RETURN(num_bits, &rest);
    *val += rest;
  }

  return kOk;
}

H264Parser::Result H264Parser::ReadSE(int* val) {
  int ue;
  Result res;

  // See Chapter 9 in the spec.
  res = ReadUE(&ue);
  if (res != kOk)
    return res;

  if (ue % 2 == 0)
    *val = -(ue / 2);
  else
    *val = ue / 2 + 1;

  return kOk;
}

H264Parser::Result H264Parser::AdvanceToNextNALU(H264NALU* nalu) {
  off_t start_code_size;
  off_t nalu_size_with_start_code;
  if (!LocateNALU(&nalu_size_with_start_code, &start_code_size)) {
    DVLOG(4) << "Could not find next NALU, bytes left in stream: "
             << bytes_left_;
    return kEOStream;
  }

  nalu->data = stream_ + start_code_size;
  nalu->size = nalu_size_with_start_code - start_code_size;
  DVLOG(4) << "NALU found: size=" << nalu_size_with_start_code;

  // Initialize bit reader at the start of found NALU.
  if (!br_.Initialize(nalu->data, nalu->size))
    return kEOStream;

  // Move parser state to after this NALU, so next time AdvanceToNextNALU
  // is called, we will effectively be skipping it;
  // other parsing functions will use the position saved
  // in bit reader for parsing, so we don't have to remember it here.
  stream_ += nalu_size_with_start_code;
  bytes_left_ -= nalu_size_with_start_code;

  // Read NALU header, skip the forbidden_zero_bit, but check for it.
  int data;
  READ_BITS_OR_RETURN(1, &data);
  TRUE_OR_RETURN(data == 0);

  READ_BITS_OR_RETURN(2, &nalu->nal_ref_idc);
  READ_BITS_OR_RETURN(5, &nalu->nal_unit_type);

  DVLOG(4) << "NALU type: " << static_cast<int>(nalu->nal_unit_type)
           << " at: " << reinterpret_cast<const void*>(nalu->data)
           << " size: " << nalu->size
           << " ref: " << static_cast<int>(nalu->nal_ref_idc);

  return kOk;
}

// Default scaling lists (per spec).
static const int kDefault4x4Intra[kH264ScalingList4x4Length] = {
    6, 13, 13, 20, 20, 20, 28, 28, 28, 28, 32, 32, 32, 37, 37, 42, };

static const int kDefault4x4Inter[kH264ScalingList4x4Length] = {
    10, 14, 14, 20, 20, 20, 24, 24, 24, 24, 27, 27, 27, 30, 30, 34, };

static const int kDefault8x8Intra[kH264ScalingList8x8Length] = {
    6,  10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
    23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
    27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
    31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42, };

static const int kDefault8x8Inter[kH264ScalingList8x8Length] = {
    9,  13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
    21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
    24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
    27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35, };

static inline void DefaultScalingList4x4(
    int i,
    int scaling_list4x4[][kH264ScalingList4x4Length]) {
  DCHECK_LT(i, 6);

  if (i < 3)
    memcpy(scaling_list4x4[i], kDefault4x4Intra, sizeof(kDefault4x4Intra));
  else if (i < 6)
    memcpy(scaling_list4x4[i], kDefault4x4Inter, sizeof(kDefault4x4Inter));
}

static inline void DefaultScalingList8x8(
    int i,
    int scaling_list8x8[][kH264ScalingList8x8Length]) {
  DCHECK_LT(i, 6);

  if (i % 2 == 0)
    memcpy(scaling_list8x8[i], kDefault8x8Intra, sizeof(kDefault8x8Intra));
  else
    memcpy(scaling_list8x8[i], kDefault8x8Inter, sizeof(kDefault8x8Inter));
}

static void FallbackScalingList4x4(
    int i,
    const int default_scaling_list_intra[],
    const int default_scaling_list_inter[],
    int scaling_list4x4[][kH264ScalingList4x4Length]) {
  static const int kScalingList4x4ByteSize =
      sizeof(scaling_list4x4[0][0]) * kH264ScalingList4x4Length;

  switch (i) {
    case 0:
      memcpy(scaling_list4x4[i], default_scaling_list_intra,
             kScalingList4x4ByteSize);
      break;

    case 1:
      memcpy(scaling_list4x4[i], scaling_list4x4[0], kScalingList4x4ByteSize);
      break;

    case 2:
      memcpy(scaling_list4x4[i], scaling_list4x4[1], kScalingList4x4ByteSize);
      break;

    case 3:
      memcpy(scaling_list4x4[i], default_scaling_list_inter,
             kScalingList4x4ByteSize);
      break;

    case 4:
      memcpy(scaling_list4x4[i], scaling_list4x4[3], kScalingList4x4ByteSize);
      break;

    case 5:
      memcpy(scaling_list4x4[i], scaling_list4x4[4], kScalingList4x4ByteSize);
      break;

    default:
      NOTREACHED();
      break;
  }
}

static void FallbackScalingList8x8(
    int i,
    const int default_scaling_list_intra[],
    const int default_scaling_list_inter[],
    int scaling_list8x8[][kH264ScalingList8x8Length]) {
  static const int kScalingList8x8ByteSize =
      sizeof(scaling_list8x8[0][0]) * kH264ScalingList8x8Length;

  switch (i) {
    case 0:
      memcpy(scaling_list8x8[i], default_scaling_list_intra,
             kScalingList8x8ByteSize);
      break;

    case 1:
      memcpy(scaling_list8x8[i], default_scaling_list_inter,
             kScalingList8x8ByteSize);
      break;

    case 2:
      memcpy(scaling_list8x8[i], scaling_list8x8[0], kScalingList8x8ByteSize);
      break;

    case 3:
      memcpy(scaling_list8x8[i], scaling_list8x8[1], kScalingList8x8ByteSize);
      break;

    case 4:
      memcpy(scaling_list8x8[i], scaling_list8x8[2], kScalingList8x8ByteSize);
      break;

    case 5:
      memcpy(scaling_list8x8[i], scaling_list8x8[3], kScalingList8x8ByteSize);
      break;

    default:
      NOTREACHED();
      break;
  }
}

H264Parser::Result H264Parser::ParseScalingList(int size,
                                                int* scaling_list,
                                                bool* use_default) {
  // See chapter 7.3.2.1.1.1.
  int last_scale = 8;
  int next_scale = 8;
  int delta_scale;

  *use_default = false;

  for (int j = 0; j < size; ++j) {
    if (next_scale != 0) {
      READ_SE_OR_RETURN(&delta_scale);
      IN_RANGE_OR_RETURN(delta_scale, -128, 127);
      next_scale = (last_scale + delta_scale + 256) & 0xff;

      if (j == 0 && next_scale == 0) {
        *use_default = true;
        return kOk;
      }
    }

    scaling_list[j] = (next_scale == 0) ? last_scale : next_scale;
    last_scale = scaling_list[j];
  }

  return kOk;
}

H264Parser::Result H264Parser::ParseSPSScalingLists(H264SPS* sps) {
  // See 7.4.2.1.1.
  bool seq_scaling_list_present_flag;
  bool use_default;
  Result res;

  // Parse scaling_list4x4.
  for (int i = 0; i < 6; ++i) {
    READ_BOOL_OR_RETURN(&seq_scaling_list_present_flag);

    if (seq_scaling_list_present_flag) {
      res = ParseScalingList(arraysize(sps->scaling_list4x4[i]),
                             sps->scaling_list4x4[i],
                             &use_default);
      if (res != kOk)
        return res;

      if (use_default)
        DefaultScalingList4x4(i, sps->scaling_list4x4);

    } else {
      FallbackScalingList4x4(
          i, kDefault4x4Intra, kDefault4x4Inter, sps->scaling_list4x4);
    }
  }

  // Parse scaling_list8x8.
  for (int i = 0; i < ((sps->chroma_format_idc != 3) ? 2 : 6); ++i) {
    READ_BOOL_OR_RETURN(&seq_scaling_list_present_flag);

    if (seq_scaling_list_present_flag) {
      res = ParseScalingList(arraysize(sps->scaling_list8x8[i]),
                             sps->scaling_list8x8[i],
                             &use_default);
      if (res != kOk)
        return res;

      if (use_default)
        DefaultScalingList8x8(i, sps->scaling_list8x8);

    } else {
      FallbackScalingList8x8(
          i, kDefault8x8Intra, kDefault8x8Inter, sps->scaling_list8x8);
    }
  }

  return kOk;
}

H264Parser::Result H264Parser::ParsePPSScalingLists(const H264SPS& sps,
                                                    H264PPS* pps) {
  // See 7.4.2.2.
  bool pic_scaling_list_present_flag;
  bool use_default;
  Result res;

  for (int i = 0; i < 6; ++i) {
    READ_BOOL_OR_RETURN(&pic_scaling_list_present_flag);

    if (pic_scaling_list_present_flag) {
      res = ParseScalingList(arraysize(pps->scaling_list4x4[i]),
                             pps->scaling_list4x4[i],
                             &use_default);
      if (res != kOk)
        return res;

      if (use_default)
        DefaultScalingList4x4(i, pps->scaling_list4x4);

    } else {
      if (sps.seq_scaling_matrix_present_flag) {
        // Table 7-2 fallback rule A in spec.
        FallbackScalingList4x4(
            i, kDefault4x4Intra, kDefault4x4Inter, pps->scaling_list4x4);
      } else {
        // Table 7-2 fallback rule B in spec.
        FallbackScalingList4x4(i,
                               sps.scaling_list4x4[0],
                               sps.scaling_list4x4[3],
                               pps->scaling_list4x4);
      }
    }
  }

  if (pps->transform_8x8_mode_flag) {
    for (int i = 0; i < ((sps.chroma_format_idc != 3) ? 2 : 6); ++i) {
      READ_BOOL_OR_RETURN(&pic_scaling_list_present_flag);

      if (pic_scaling_list_present_flag) {
        res = ParseScalingList(arraysize(pps->scaling_list8x8[i]),
                               pps->scaling_list8x8[i],
                               &use_default);
        if (res != kOk)
          return res;

        if (use_default)
          DefaultScalingList8x8(i, pps->scaling_list8x8);

      } else {
        if (sps.seq_scaling_matrix_present_flag) {
          // Table 7-2 fallback rule A in spec.
          FallbackScalingList8x8(
              i, kDefault8x8Intra, kDefault8x8Inter, pps->scaling_list8x8);
        } else {
          // Table 7-2 fallback rule B in spec.
          FallbackScalingList8x8(i,
                                 sps.scaling_list8x8[0],
                                 sps.scaling_list8x8[1],
                                 pps->scaling_list8x8);
        }
      }
    }
  }
  return kOk;
}

H264Parser::Result H264Parser::ParseAndIgnoreHRDParameters(
    bool* hrd_parameters_present) {
  int data;
  READ_BOOL_OR_RETURN(&data);  // {nal,vcl}_hrd_parameters_present_flag
  if (!data)
    return kOk;

  *hrd_parameters_present = true;

  int cpb_cnt_minus1;
  READ_UE_OR_RETURN(&cpb_cnt_minus1);
  IN_RANGE_OR_RETURN(cpb_cnt_minus1, 0, 31);
  READ_BITS_OR_RETURN(8, &data);  // bit_rate_scale, cpb_size_scale
  for (int i = 0; i <= cpb_cnt_minus1; ++i) {
    READ_UE_OR_RETURN(&data);  // bit_rate_value_minus1[i]
    READ_UE_OR_RETURN(&data);  // cpb_size_value_minus1[i]
    READ_BOOL_OR_RETURN(&data);  // cbr_flag
  }
  READ_BITS_OR_RETURN(20, &data);  // cpb/dpb delays, etc.

  return kOk;
}

H264Parser::Result H264Parser::ParseVUIParameters(H264SPS* sps) {
  bool aspect_ratio_info_present_flag;
  READ_BOOL_OR_RETURN(&aspect_ratio_info_present_flag);
  if (aspect_ratio_info_present_flag) {
    int aspect_ratio_idc;
    READ_BITS_OR_RETURN(8, &aspect_ratio_idc);
    if (aspect_ratio_idc == kExtendedSar) {
      READ_BITS_OR_RETURN(16, &sps->sar_width);
      READ_BITS_OR_RETURN(16, &sps->sar_height);
    } else {
      const int max_aspect_ratio_idc = arraysize(kTableSarWidth) - 1;
      IN_RANGE_OR_RETURN(aspect_ratio_idc, 0, max_aspect_ratio_idc);
      sps->sar_width = kTableSarWidth[aspect_ratio_idc];
      sps->sar_height = kTableSarHeight[aspect_ratio_idc];
    }
  }

  int data;
  // Read and ignore overscan and video signal type info.
  READ_BOOL_OR_RETURN(&data);  // overscan_info_present_flag
  if (data)
    READ_BOOL_OR_RETURN(&data);  // overscan_appropriate_flag

  READ_BOOL_OR_RETURN(&data);  // video_signal_type_present_flag
  if (data) {
    READ_BITS_OR_RETURN(3, &data);  // video_format
    READ_BOOL_OR_RETURN(&data);  // video_full_range_flag
    READ_BOOL_OR_RETURN(&data);  // colour_description_present_flag
    if (data)
      READ_BITS_OR_RETURN(24, &data);  // color description syntax elements
  }

  READ_BOOL_OR_RETURN(&data);  // chroma_loc_info_present_flag
  if (data) {
    READ_UE_OR_RETURN(&data);  // chroma_sample_loc_type_top_field
    READ_UE_OR_RETURN(&data);  // chroma_sample_loc_type_bottom_field
  }

  // Read and ignore timing info.
  READ_BOOL_OR_RETURN(&data);  // timing_info_present_flag
  if (data) {
    READ_BITS_OR_RETURN(16, &data);  // num_units_in_tick
    READ_BITS_OR_RETURN(16, &data);  // num_units_in_tick
    READ_BITS_OR_RETURN(16, &data);  // time_scale
    READ_BITS_OR_RETURN(16, &data);  // time_scale
    READ_BOOL_OR_RETURN(&data);  // fixed_frame_rate_flag
  }

  // Read and ignore NAL HRD parameters, if present.
  bool hrd_parameters_present = false;
  Result res = ParseAndIgnoreHRDParameters(&hrd_parameters_present);
  if (res != kOk)
    return res;

  // Read and ignore VCL HRD parameters, if present.
  res = ParseAndIgnoreHRDParameters(&hrd_parameters_present);
  if (res != kOk)
    return res;

  if (hrd_parameters_present)  // One of NAL or VCL params present is enough.
    READ_BOOL_OR_RETURN(&data);  // low_delay_hrd_flag

  READ_BOOL_OR_RETURN(&data);  // pic_struct_present_flag
  READ_BOOL_OR_RETURN(&sps->bitstream_restriction_flag);
  if (sps->bitstream_restriction_flag) {
    READ_BOOL_OR_RETURN(&data);  // motion_vectors_over_pic_boundaries_flag
    READ_UE_OR_RETURN(&data);  // max_bytes_per_pic_denom
    READ_UE_OR_RETURN(&data);  // max_bits_per_mb_denom
    READ_UE_OR_RETURN(&data);  // log2_max_mv_length_horizontal
    READ_UE_OR_RETURN(&data);  // log2_max_mv_length_vertical
    READ_UE_OR_RETURN(&sps->max_num_reorder_frames);
    READ_UE_OR_RETURN(&sps->max_dec_frame_buffering);
    TRUE_OR_RETURN(sps->max_dec_frame_buffering >= sps->max_num_ref_frames);
    IN_RANGE_OR_RETURN(
        sps->max_num_reorder_frames, 0, sps->max_dec_frame_buffering);
  }

  return kOk;
}

static void FillDefaultSeqScalingLists(H264SPS* sps) {
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < kH264ScalingList4x4Length; ++j)
      sps->scaling_list4x4[i][j] = 16;

  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < kH264ScalingList8x8Length; ++j)
      sps->scaling_list8x8[i][j] = 16;
}

H264Parser::Result H264Parser::ParseSPS(int* sps_id) {
  // See 7.4.2.1.
  int data;
  Result res;

  *sps_id = -1;

  scoped_ptr<H264SPS> sps(new H264SPS());

  READ_BITS_OR_RETURN(8, &sps->profile_idc);
  READ_BOOL_OR_RETURN(&sps->constraint_set0_flag);
  READ_BOOL_OR_RETURN(&sps->constraint_set1_flag);
  READ_BOOL_OR_RETURN(&sps->constraint_set2_flag);
  READ_BOOL_OR_RETURN(&sps->constraint_set3_flag);
  READ_BOOL_OR_RETURN(&sps->constraint_set4_flag);
  READ_BOOL_OR_RETURN(&sps->constraint_set5_flag);
  READ_BITS_OR_RETURN(2, &data);  // reserved_zero_2bits
  READ_BITS_OR_RETURN(8, &sps->level_idc);
  READ_UE_OR_RETURN(&sps->seq_parameter_set_id);
  TRUE_OR_RETURN(sps->seq_parameter_set_id < 32);

  if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
      sps->profile_idc == 122 || sps->profile_idc == 244 ||
      sps->profile_idc == 44 || sps->profile_idc == 83 ||
      sps->profile_idc == 86 || sps->profile_idc == 118 ||
      sps->profile_idc == 128) {
    READ_UE_OR_RETURN(&sps->chroma_format_idc);
    TRUE_OR_RETURN(sps->chroma_format_idc < 4);

    if (sps->chroma_format_idc == 3)
      READ_BOOL_OR_RETURN(&sps->separate_colour_plane_flag);

    READ_UE_OR_RETURN(&sps->bit_depth_luma_minus8);
    TRUE_OR_RETURN(sps->bit_depth_luma_minus8 < 7);

    READ_UE_OR_RETURN(&sps->bit_depth_chroma_minus8);
    TRUE_OR_RETURN(sps->bit_depth_chroma_minus8 < 7);

    READ_BOOL_OR_RETURN(&sps->qpprime_y_zero_transform_bypass_flag);
    READ_BOOL_OR_RETURN(&sps->seq_scaling_matrix_present_flag);

    if (sps->seq_scaling_matrix_present_flag) {
      DVLOG(4) << "Scaling matrix present";
      res = ParseSPSScalingLists(sps.get());
      if (res != kOk)
        return res;
    } else {
      FillDefaultSeqScalingLists(sps.get());
    }
  } else {
    sps->chroma_format_idc = 1;
    FillDefaultSeqScalingLists(sps.get());
  }

  if (sps->separate_colour_plane_flag)
    sps->chroma_array_type = 0;
  else
    sps->chroma_array_type = sps->chroma_format_idc;

  READ_UE_OR_RETURN(&sps->log2_max_frame_num_minus4);
  TRUE_OR_RETURN(sps->log2_max_frame_num_minus4 < 13);

  READ_UE_OR_RETURN(&sps->pic_order_cnt_type);
  TRUE_OR_RETURN(sps->pic_order_cnt_type < 3);

  sps->expected_delta_per_pic_order_cnt_cycle = 0;
  if (sps->pic_order_cnt_type == 0) {
    READ_UE_OR_RETURN(&sps->log2_max_pic_order_cnt_lsb_minus4);
    TRUE_OR_RETURN(sps->log2_max_pic_order_cnt_lsb_minus4 < 13);
  } else if (sps->pic_order_cnt_type == 1) {
    READ_BOOL_OR_RETURN(&sps->delta_pic_order_always_zero_flag);
    READ_SE_OR_RETURN(&sps->offset_for_non_ref_pic);
    READ_SE_OR_RETURN(&sps->offset_for_top_to_bottom_field);
    READ_UE_OR_RETURN(&sps->num_ref_frames_in_pic_order_cnt_cycle);
    TRUE_OR_RETURN(sps->num_ref_frames_in_pic_order_cnt_cycle < 255);

    for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; ++i) {
      READ_SE_OR_RETURN(&sps->offset_for_ref_frame[i]);
      sps->expected_delta_per_pic_order_cnt_cycle +=
          sps->offset_for_ref_frame[i];
    }
  }

  READ_UE_OR_RETURN(&sps->max_num_ref_frames);
  READ_BOOL_OR_RETURN(&sps->gaps_in_frame_num_value_allowed_flag);

  if (sps->gaps_in_frame_num_value_allowed_flag)
    return kUnsupportedStream;

  READ_UE_OR_RETURN(&sps->pic_width_in_mbs_minus1);
  READ_UE_OR_RETURN(&sps->pic_height_in_map_units_minus1);

  READ_BOOL_OR_RETURN(&sps->frame_mbs_only_flag);
  if (!sps->frame_mbs_only_flag)
    READ_BOOL_OR_RETURN(&sps->mb_adaptive_frame_field_flag);

  READ_BOOL_OR_RETURN(&sps->direct_8x8_inference_flag);

  READ_BOOL_OR_RETURN(&sps->frame_cropping_flag);
  if (sps->frame_cropping_flag) {
    READ_UE_OR_RETURN(&sps->frame_crop_left_offset);
    READ_UE_OR_RETURN(&sps->frame_crop_right_offset);
    READ_UE_OR_RETURN(&sps->frame_crop_top_offset);
    READ_UE_OR_RETURN(&sps->frame_crop_bottom_offset);
  }

  READ_BOOL_OR_RETURN(&sps->vui_parameters_present_flag);
  if (sps->vui_parameters_present_flag) {
    DVLOG(4) << "VUI parameters present";
    res = ParseVUIParameters(sps.get());
    if (res != kOk)
      return res;
  }

  // If an SPS with the same id already exists, replace it.
  *sps_id = sps->seq_parameter_set_id;
  delete active_SPSes_[*sps_id];
  active_SPSes_[*sps_id] = sps.release();

  return kOk;
}

H264Parser::Result H264Parser::ParsePPS(int* pps_id) {
  // See 7.4.2.2.
  const H264SPS* sps;
  Result res;

  *pps_id = -1;

  scoped_ptr<H264PPS> pps(new H264PPS());

  READ_UE_OR_RETURN(&pps->pic_parameter_set_id);
  READ_UE_OR_RETURN(&pps->seq_parameter_set_id);
  TRUE_OR_RETURN(pps->seq_parameter_set_id < 32);

  sps = GetSPS(pps->seq_parameter_set_id);
  TRUE_OR_RETURN(sps);

  READ_BOOL_OR_RETURN(&pps->entropy_coding_mode_flag);
  READ_BOOL_OR_RETURN(&pps->bottom_field_pic_order_in_frame_present_flag);

  READ_UE_OR_RETURN(&pps->num_slice_groups_minus1);
  if (pps->num_slice_groups_minus1 > 1) {
    DVLOG(1) << "Slice groups not supported";
    return kUnsupportedStream;
  }

  READ_UE_OR_RETURN(&pps->num_ref_idx_l0_default_active_minus1);
  TRUE_OR_RETURN(pps->num_ref_idx_l0_default_active_minus1 < 32);

  READ_UE_OR_RETURN(&pps->num_ref_idx_l1_default_active_minus1);
  TRUE_OR_RETURN(pps->num_ref_idx_l1_default_active_minus1 < 32);

  READ_BOOL_OR_RETURN(&pps->weighted_pred_flag);
  READ_BITS_OR_RETURN(2, &pps->weighted_bipred_idc);
  TRUE_OR_RETURN(pps->weighted_bipred_idc < 3);

  READ_SE_OR_RETURN(&pps->pic_init_qp_minus26);
  IN_RANGE_OR_RETURN(pps->pic_init_qp_minus26, -26, 25);

  READ_SE_OR_RETURN(&pps->pic_init_qs_minus26);
  IN_RANGE_OR_RETURN(pps->pic_init_qs_minus26, -26, 25);

  READ_SE_OR_RETURN(&pps->chroma_qp_index_offset);
  IN_RANGE_OR_RETURN(pps->chroma_qp_index_offset, -12, 12);
  pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;

  READ_BOOL_OR_RETURN(&pps->deblocking_filter_control_present_flag);
  READ_BOOL_OR_RETURN(&pps->constrained_intra_pred_flag);
  READ_BOOL_OR_RETURN(&pps->redundant_pic_cnt_present_flag);

  if (br_.HasMoreRBSPData()) {
    READ_BOOL_OR_RETURN(&pps->transform_8x8_mode_flag);
    READ_BOOL_OR_RETURN(&pps->pic_scaling_matrix_present_flag);

    if (pps->pic_scaling_matrix_present_flag) {
      DVLOG(4) << "Picture scaling matrix present";
      res = ParsePPSScalingLists(*sps, pps.get());
      if (res != kOk)
        return res;
    }

    READ_SE_OR_RETURN(&pps->second_chroma_qp_index_offset);
  }

  // If a PPS with the same id already exists, replace it.
  *pps_id = pps->pic_parameter_set_id;
  delete active_PPSes_[*pps_id];
  active_PPSes_[*pps_id] = pps.release();

  return kOk;
}

H264Parser::Result H264Parser::ParseRefPicListModification(
    int num_ref_idx_active_minus1,
    H264ModificationOfPicNum* ref_list_mods) {
  H264ModificationOfPicNum* pic_num_mod;

  if (num_ref_idx_active_minus1 >= 32)
    return kInvalidStream;

  for (int i = 0; i < 32; ++i) {
    pic_num_mod = &ref_list_mods[i];
    READ_UE_OR_RETURN(&pic_num_mod->modification_of_pic_nums_idc);
    TRUE_OR_RETURN(pic_num_mod->modification_of_pic_nums_idc < 4);

    switch (pic_num_mod->modification_of_pic_nums_idc) {
      case 0:
      case 1:
        READ_UE_OR_RETURN(&pic_num_mod->abs_diff_pic_num_minus1);
        break;

      case 2:
        READ_UE_OR_RETURN(&pic_num_mod->long_term_pic_num);
        break;

      case 3:
        // Per spec, list cannot be empty.
        if (i == 0)
          return kInvalidStream;
        return kOk;

      default:
        return kInvalidStream;
    }
  }

  // If we got here, we didn't get loop end marker prematurely,
  // so make sure it is there for our client.
  int modification_of_pic_nums_idc;
  READ_UE_OR_RETURN(&modification_of_pic_nums_idc);
  TRUE_OR_RETURN(modification_of_pic_nums_idc == 3);

  return kOk;
}

H264Parser::Result H264Parser::ParseRefPicListModifications(
    H264SliceHeader* shdr) {
  Result res;

  if (!shdr->IsISlice() && !shdr->IsSISlice()) {
    READ_BOOL_OR_RETURN(&shdr->ref_pic_list_modification_flag_l0);
    if (shdr->ref_pic_list_modification_flag_l0) {
      res = ParseRefPicListModification(shdr->num_ref_idx_l0_active_minus1,
                                        shdr->ref_list_l0_modifications);
      if (res != kOk)
        return res;
    }
  }

  if (shdr->IsBSlice()) {
    READ_BOOL_OR_RETURN(&shdr->ref_pic_list_modification_flag_l1);
    if (shdr->ref_pic_list_modification_flag_l1) {
      res = ParseRefPicListModification(shdr->num_ref_idx_l1_active_minus1,
                                        shdr->ref_list_l1_modifications);
      if (res != kOk)
        return res;
    }
  }

  return kOk;
}

H264Parser::Result H264Parser::ParseWeightingFactors(
    int num_ref_idx_active_minus1,
    int chroma_array_type,
    int luma_log2_weight_denom,
    int chroma_log2_weight_denom,
    H264WeightingFactors* w_facts) {

  int def_luma_weight = 1 << luma_log2_weight_denom;
  int def_chroma_weight = 1 << chroma_log2_weight_denom;

  for (int i = 0; i < num_ref_idx_active_minus1 + 1; ++i) {
    READ_BOOL_OR_RETURN(&w_facts->luma_weight_flag);
    if (w_facts->luma_weight_flag) {
      READ_SE_OR_RETURN(&w_facts->luma_weight[i]);
      IN_RANGE_OR_RETURN(w_facts->luma_weight[i], -128, 127);

      READ_SE_OR_RETURN(&w_facts->luma_offset[i]);
      IN_RANGE_OR_RETURN(w_facts->luma_offset[i], -128, 127);
    } else {
      w_facts->luma_weight[i] = def_luma_weight;
      w_facts->luma_offset[i] = 0;
    }

    if (chroma_array_type != 0) {
      READ_BOOL_OR_RETURN(&w_facts->chroma_weight_flag);
      if (w_facts->chroma_weight_flag) {
        for (int j = 0; j < 2; ++j) {
          READ_SE_OR_RETURN(&w_facts->chroma_weight[i][j]);
          IN_RANGE_OR_RETURN(w_facts->chroma_weight[i][j], -128, 127);

          READ_SE_OR_RETURN(&w_facts->chroma_offset[i][j]);
          IN_RANGE_OR_RETURN(w_facts->chroma_offset[i][j], -128, 127);
        }
      } else {
        for (int j = 0; j < 2; ++j) {
          w_facts->chroma_weight[i][j] = def_chroma_weight;
          w_facts->chroma_offset[i][j] = 0;
        }
      }
    }
  }

  return kOk;
}

H264Parser::Result H264Parser::ParsePredWeightTable(const H264SPS& sps,
                                                    H264SliceHeader* shdr) {
  READ_UE_OR_RETURN(&shdr->luma_log2_weight_denom);
  TRUE_OR_RETURN(shdr->luma_log2_weight_denom < 8);

  if (sps.chroma_array_type != 0)
    READ_UE_OR_RETURN(&shdr->chroma_log2_weight_denom);
  TRUE_OR_RETURN(shdr->chroma_log2_weight_denom < 8);

  Result res = ParseWeightingFactors(shdr->num_ref_idx_l0_active_minus1,
                                     sps.chroma_array_type,
                                     shdr->luma_log2_weight_denom,
                                     shdr->chroma_log2_weight_denom,
                                     &shdr->pred_weight_table_l0);
  if (res != kOk)
    return res;

  if (shdr->IsBSlice()) {
    res = ParseWeightingFactors(shdr->num_ref_idx_l1_active_minus1,
                                sps.chroma_array_type,
                                shdr->luma_log2_weight_denom,
                                shdr->chroma_log2_weight_denom,
                                &shdr->pred_weight_table_l1);
    if (res != kOk)
      return res;
  }

  return kOk;
}

H264Parser::Result H264Parser::ParseDecRefPicMarking(H264SliceHeader* shdr) {
  if (shdr->idr_pic_flag) {
    READ_BOOL_OR_RETURN(&shdr->no_output_of_prior_pics_flag);
    READ_BOOL_OR_RETURN(&shdr->long_term_reference_flag);
  } else {
    READ_BOOL_OR_RETURN(&shdr->adaptive_ref_pic_marking_mode_flag);

    H264DecRefPicMarking* marking;
    if (shdr->adaptive_ref_pic_marking_mode_flag) {
      size_t i;
      for (i = 0; i < arraysize(shdr->ref_pic_marking); ++i) {
        marking = &shdr->ref_pic_marking[i];

        READ_UE_OR_RETURN(&marking->memory_mgmnt_control_operation);
        if (marking->memory_mgmnt_control_operation == 0)
          break;

        if (marking->memory_mgmnt_control_operation == 1 ||
            marking->memory_mgmnt_control_operation == 3)
          READ_UE_OR_RETURN(&marking->difference_of_pic_nums_minus1);

        if (marking->memory_mgmnt_control_operation == 2)
          READ_UE_OR_RETURN(&marking->long_term_pic_num);

        if (marking->memory_mgmnt_control_operation == 3 ||
            marking->memory_mgmnt_control_operation == 6)
          READ_UE_OR_RETURN(&marking->long_term_frame_idx);

        if (marking->memory_mgmnt_control_operation == 4)
          READ_UE_OR_RETURN(&marking->max_long_term_frame_idx_plus1);

        if (marking->memory_mgmnt_control_operation > 6)
          return kInvalidStream;
      }

      if (i == arraysize(shdr->ref_pic_marking)) {
        DVLOG(1) << "Ran out of dec ref pic marking fields";
        return kUnsupportedStream;
      }
    }
  }

  return kOk;
}

H264Parser::Result H264Parser::ParseSliceHeader(const H264NALU& nalu,
                                                H264SliceHeader* shdr) {
  // See 7.4.3.
  const H264SPS* sps;
  const H264PPS* pps;
  Result res;

  memset(shdr, 0, sizeof(*shdr));

  shdr->idr_pic_flag = (nalu.nal_unit_type == 5);
  shdr->nal_ref_idc = nalu.nal_ref_idc;
  shdr->nalu_data = nalu.data;
  shdr->nalu_size = nalu.size;

  READ_UE_OR_RETURN(&shdr->first_mb_in_slice);
  READ_UE_OR_RETURN(&shdr->slice_type);
  TRUE_OR_RETURN(shdr->slice_type < 10);

  READ_UE_OR_RETURN(&shdr->pic_parameter_set_id);

  pps = GetPPS(shdr->pic_parameter_set_id);
  TRUE_OR_RETURN(pps);

  sps = GetSPS(pps->seq_parameter_set_id);
  TRUE_OR_RETURN(sps);

  if (sps->separate_colour_plane_flag) {
    DVLOG(1) << "Interlaced streams not supported";
    return kUnsupportedStream;
  }

  READ_BITS_OR_RETURN(sps->log2_max_frame_num_minus4 + 4, &shdr->frame_num);
  if (!sps->frame_mbs_only_flag) {
    READ_BOOL_OR_RETURN(&shdr->field_pic_flag);
    if (shdr->field_pic_flag) {
      DVLOG(1) << "Interlaced streams not supported";
      return kUnsupportedStream;
    }
  }

  if (shdr->idr_pic_flag)
    READ_UE_OR_RETURN(&shdr->idr_pic_id);

  if (sps->pic_order_cnt_type == 0) {
    READ_BITS_OR_RETURN(sps->log2_max_pic_order_cnt_lsb_minus4 + 4,
                        &shdr->pic_order_cnt_lsb);
    if (pps->bottom_field_pic_order_in_frame_present_flag &&
        !shdr->field_pic_flag)
      READ_SE_OR_RETURN(&shdr->delta_pic_order_cnt_bottom);
  }

  if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
    READ_SE_OR_RETURN(&shdr->delta_pic_order_cnt[0]);
    if (pps->bottom_field_pic_order_in_frame_present_flag &&
        !shdr->field_pic_flag)
      READ_SE_OR_RETURN(&shdr->delta_pic_order_cnt[1]);
  }

  if (pps->redundant_pic_cnt_present_flag) {
    READ_UE_OR_RETURN(&shdr->redundant_pic_cnt);
    TRUE_OR_RETURN(shdr->redundant_pic_cnt < 128);
  }

  if (shdr->IsBSlice())
    READ_BOOL_OR_RETURN(&shdr->direct_spatial_mv_pred_flag);

  if (shdr->IsPSlice() || shdr->IsSPSlice() || shdr->IsBSlice()) {
    READ_BOOL_OR_RETURN(&shdr->num_ref_idx_active_override_flag);
    if (shdr->num_ref_idx_active_override_flag) {
      READ_UE_OR_RETURN(&shdr->num_ref_idx_l0_active_minus1);
      if (shdr->IsBSlice())
        READ_UE_OR_RETURN(&shdr->num_ref_idx_l1_active_minus1);
    } else {
      shdr->num_ref_idx_l0_active_minus1 =
          pps->num_ref_idx_l0_default_active_minus1;
      if (shdr->IsBSlice()) {
        shdr->num_ref_idx_l1_active_minus1 =
            pps->num_ref_idx_l1_default_active_minus1;
      }
    }
  }
  if (shdr->field_pic_flag) {
    TRUE_OR_RETURN(shdr->num_ref_idx_l0_active_minus1 < 32);
    TRUE_OR_RETURN(shdr->num_ref_idx_l1_active_minus1 < 32);
  } else {
    TRUE_OR_RETURN(shdr->num_ref_idx_l0_active_minus1 < 16);
    TRUE_OR_RETURN(shdr->num_ref_idx_l1_active_minus1 < 16);
  }

  if (nalu.nal_unit_type == H264NALU::kCodedSliceExtension) {
    return kUnsupportedStream;
  } else {
    res = ParseRefPicListModifications(shdr);
    if (res != kOk)
      return res;
  }

  if ((pps->weighted_pred_flag && (shdr->IsPSlice() || shdr->IsSPSlice())) ||
      (pps->weighted_bipred_idc == 1 && shdr->IsBSlice())) {
    res = ParsePredWeightTable(*sps, shdr);
    if (res != kOk)
      return res;
  }

  if (nalu.nal_ref_idc != 0) {
    res = ParseDecRefPicMarking(shdr);
    if (res != kOk)
      return res;
  }

  if (pps->entropy_coding_mode_flag && !shdr->IsISlice() &&
      !shdr->IsSISlice()) {
    READ_UE_OR_RETURN(&shdr->cabac_init_idc);
    TRUE_OR_RETURN(shdr->cabac_init_idc < 3);
  }

  READ_SE_OR_RETURN(&shdr->slice_qp_delta);

  if (shdr->IsSPSlice() || shdr->IsSISlice()) {
    if (shdr->IsSPSlice())
      READ_BOOL_OR_RETURN(&shdr->sp_for_switch_flag);
    READ_SE_OR_RETURN(&shdr->slice_qs_delta);
  }

  if (pps->deblocking_filter_control_present_flag) {
    READ_UE_OR_RETURN(&shdr->disable_deblocking_filter_idc);
    TRUE_OR_RETURN(shdr->disable_deblocking_filter_idc < 3);

    if (shdr->disable_deblocking_filter_idc != 1) {
      READ_SE_OR_RETURN(&shdr->slice_alpha_c0_offset_div2);
      IN_RANGE_OR_RETURN(shdr->slice_alpha_c0_offset_div2, -6, 6);

      READ_SE_OR_RETURN(&shdr->slice_beta_offset_div2);
      IN_RANGE_OR_RETURN(shdr->slice_beta_offset_div2, -6, 6);
    }
  }

  if (pps->num_slice_groups_minus1 > 0) {
    DVLOG(1) << "Slice groups not supported";
    return kUnsupportedStream;
  }

  size_t epb = br_.NumEmulationPreventionBytesRead();
  shdr->header_bit_size = (shdr->nalu_size - epb) * 8 - br_.NumBitsLeft();

  return kOk;
}

H264Parser::Result H264Parser::ParseSEI(H264SEIMessage* sei_msg) {
  int byte;

  memset(sei_msg, 0, sizeof(*sei_msg));

  READ_BITS_OR_RETURN(8, &byte);
  while (byte == 0xff) {
    sei_msg->type += 255;
    READ_BITS_OR_RETURN(8, &byte);
  }
  sei_msg->type += byte;

  READ_BITS_OR_RETURN(8, &byte);
  while (byte == 0xff) {
    sei_msg->payload_size += 255;
    READ_BITS_OR_RETURN(8, &byte);
  }
  sei_msg->payload_size += byte;

  DVLOG(4) << "Found SEI message type: " << sei_msg->type
           << " payload size: " << sei_msg->payload_size;

  switch (sei_msg->type) {
    case H264SEIMessage::kSEIRecoveryPoint:
      READ_UE_OR_RETURN(&sei_msg->recovery_point.recovery_frame_cnt);
      READ_BOOL_OR_RETURN(&sei_msg->recovery_point.exact_match_flag);
      READ_BOOL_OR_RETURN(&sei_msg->recovery_point.broken_link_flag);
      READ_BITS_OR_RETURN(2, &sei_msg->recovery_point.changing_slice_group_idc);
      break;

    default:
      DVLOG(4) << "Unsupported SEI message";
      break;
  }

  return kOk;
}

}  // namespace media
