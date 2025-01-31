// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/rtp_receiver/rtp_parser/rtp_parser.h"

#include "base/big_endian.h"
#include "base/logging.h"
#include "media/cast/cast_defines.h"

namespace media {
namespace cast {

static const size_t kRtpHeaderLength = 12;
static const size_t kCastHeaderLength = 7;
static const uint8 kRtpExtensionBitMask = 0x10;
static const uint8 kRtpMarkerBitMask = 0x80;
static const uint8 kCastKeyFrameBitMask = 0x80;
static const uint8 kCastReferenceFrameIdBitMask = 0x40;

RtpParser::RtpParser(uint32 expected_sender_ssrc, uint8 expected_payload_type)
    : expected_sender_ssrc_(expected_sender_ssrc),
      expected_payload_type_(expected_payload_type) {}

RtpParser::~RtpParser() {}

bool RtpParser::ParsePacket(const uint8* packet,
                            size_t length,
                            RtpCastHeader* header,
                            const uint8** payload_data,
                            size_t* payload_size) {
  DCHECK(packet);
  DCHECK(header);
  DCHECK(payload_data);
  DCHECK(payload_size);

  if (length < (kRtpHeaderLength + kCastHeaderLength))
    return false;

  base::BigEndianReader reader(reinterpret_cast<const char*>(packet), length);

  // Parse the RTP header.  See
  // http://en.wikipedia.org/wiki/Real-time_Transport_Protocol for an
  // explanation of the standard RTP packet header.
  uint8 bits;
  if (!reader.ReadU8(&bits))
    return false;
  const uint8 version = bits >> 6;
  if (version != 2)
    return false;
  if (bits & kRtpExtensionBitMask)
    return false;  // We lack the implementation to skip over an extension.
  if (!reader.ReadU8(&bits))
    return false;
  header->marker = !!(bits & kRtpMarkerBitMask);
  header->payload_type = bits & ~kRtpMarkerBitMask;
  if (header->payload_type != expected_payload_type_)
    return false;  // Punt: Unexpected payload type.
  if (!reader.ReadU16(&header->sequence_number) ||
      !reader.ReadU32(&header->rtp_timestamp) ||
      !reader.ReadU32(&header->sender_ssrc)) {
    return false;
  }
  if (header->sender_ssrc != expected_sender_ssrc_)
    return false;  // Punt: Sender's SSRC does not match the expected one.

  // Parse the Cast header.  Note that, from the RTP protocol's perspective, the
  // Cast header is part of the payload (and not meant to be an extension
  // header).
  if (!reader.ReadU8(&bits))
    return false;
  header->is_key_frame = !!(bits & kCastKeyFrameBitMask);
  const bool includes_specific_frame_reference =
      !!(bits & kCastReferenceFrameIdBitMask);
  uint8 truncated_frame_id;
  if (!reader.ReadU8(&truncated_frame_id) ||
      !reader.ReadU16(&header->packet_id) ||
      !reader.ReadU16(&header->max_packet_id)) {
    return false;
  }
  // Sanity-check: Do the packet ID values make sense w.r.t. each other?
  if (header->max_packet_id < header->packet_id)
    return false;
  uint8 truncated_reference_frame_id;
  if (!includes_specific_frame_reference) {
    // By default, a key frame only references itself; and non-key frames
    // reference their direct predecessor.
    truncated_reference_frame_id = truncated_frame_id;
    if (!header->is_key_frame)
      --truncated_reference_frame_id;
  } else if (!reader.ReadU8(&truncated_reference_frame_id)) {
    return false;
  }

  // Only the lower 8 bits of the |frame_id| were serialized, so do some magic
  // to restore the upper 24 bits.
  //
  // Note: The call to |frame_id_wrap_helper_| has side effects, so we must not
  // call it until we know the entire deserialization will succeed.
  header->frame_id =
      frame_id_wrap_helper_.MapTo32bitsFrameId(truncated_frame_id);
  // When the upper 24 bits are restored to |reference_frame_id|, make sure
  // |reference_frame_id| will be strictly less than or equal to |frame_id|.
  if (truncated_reference_frame_id <= truncated_frame_id)
    header->reference_frame_id = header->frame_id & 0xffffff00;
  else
    header->reference_frame_id = (header->frame_id & 0xffffff00) - 0x00000100;
  header->reference_frame_id |= truncated_reference_frame_id;

  // All remaining data in the packet is the payload.
  *payload_data = reinterpret_cast<const uint8*>(reader.ptr());
  *payload_size = reader.remaining();

  return true;
}

}  // namespace cast
}  // namespace media
