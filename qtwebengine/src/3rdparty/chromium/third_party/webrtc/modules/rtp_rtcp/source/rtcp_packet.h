/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_RTCP_PACKET_H_
#define WEBRTC_MODULES_RTP_RTCP_RTCP_PACKET_H_

#include <map>
#include <string>
#include <vector>

#include "webrtc/modules/rtp_rtcp/source/rtcp_utility.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace rtcp {

enum { kCommonFbFmtLength = 12 };
enum { kReportBlockLength = 24 };

class RawPacket;

// Class for building RTCP packets.
//
//  Example:
//  ReportBlock report_block;
//  report_block.To(234)
//  report_block.FractionLost(10);
//
//  ReceiverReport rr;
//  rr.From(123);
//  rr.WithReportBlock(&report_block)
//
//  Fir fir;
//  fir.From(123);
//  fir.To(234)
//  fir.WithCommandSeqNum(123);
//
//  size_t length = 0;                     // Builds an intra frame request
//  uint8_t packet[kPacketSize];           // with sequence number 123.
//  fir.Build(packet, &length, kPacketSize);
//
//  RawPacket packet = fir.Build();        // Returns a RawPacket holding
//                                         // the built rtcp packet.
//
//  rr.Append(&fir)                        // Builds a compound RTCP packet with
//  RawPacket packet = rr.Build();         // a receiver report, report block
//                                         // and fir message.

class RtcpPacket {
 public:
  virtual ~RtcpPacket() {}

  void Append(RtcpPacket* packet);

  RawPacket Build() const;

  void Build(uint8_t* packet, size_t* length, size_t max_length) const;

 protected:
  RtcpPacket() : kHeaderLength(4) {}

  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const = 0;

  const size_t kHeaderLength;

 private:
  void CreateAndAddAppended(
      uint8_t* packet, size_t* length, size_t max_length) const;

  std::vector<RtcpPacket*> appended_packets_;
};

class Empty : public RtcpPacket {
 public:
  Empty() {}

  virtual ~Empty() {}

 protected:
  virtual void Create(uint8_t* packet, size_t* length, size_t max_length) const;
};

// From RFC 3550, RTP: A Transport Protocol for Real-Time Applications.
//
// RTCP report block (RFC 3550).
//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |                 SSRC_1 (SSRC of first source)                 |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  | fraction lost |       cumulative number of packets lost       |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |           extended highest sequence number received           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                      interarrival jitter                      |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                         last SR (LSR)                         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                   delay since last SR (DLSR)                  |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

class ReportBlock {
 public:
  ReportBlock() {
    // TODO(asapersson): Consider adding a constructor to struct.
    memset(&report_block_, 0, sizeof(report_block_));
  }

  ~ReportBlock() {}

  void To(uint32_t ssrc) {
    report_block_.SSRC = ssrc;
  }
  void WithFractionLost(uint8_t fraction_lost) {
    report_block_.FractionLost = fraction_lost;
  }
  void WithCumulativeLost(uint32_t cumulative_lost) {
    report_block_.CumulativeNumOfPacketsLost = cumulative_lost;
  }
  void WithExtHighestSeqNum(uint32_t ext_highest_seq_num) {
    report_block_.ExtendedHighestSequenceNumber = ext_highest_seq_num;
  }
  void WithJitter(uint32_t jitter) {
    report_block_.Jitter = jitter;
  }
  void WithLastSr(uint32_t last_sr) {
    report_block_.LastSR = last_sr;
  }
  void WithDelayLastSr(uint32_t delay_last_sr) {
    report_block_.DelayLastSR = delay_last_sr;
  }

 private:
  friend class SenderReport;
  friend class ReceiverReport;
  RTCPUtility::RTCPPacketReportBlockItem report_block_;
};

// RTCP sender report (RFC 3550).
//
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P|    RC   |   PT=SR=200   |             length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                         SSRC of sender                        |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |              NTP timestamp, most significant word             |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |             NTP timestamp, least significant word             |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                         RTP timestamp                         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                     sender's packet count                     |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                      sender's octet count                     |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |                         report block(s)                       |
//  |                            ....                               |

class SenderReport : public RtcpPacket {
 public:
  SenderReport() : RtcpPacket() {
    memset(&sr_, 0, sizeof(sr_));
  }

  virtual ~SenderReport() {}

  void From(uint32_t ssrc) {
    sr_.SenderSSRC = ssrc;
  }
  void WithNtpSec(uint32_t sec) {
    sr_.NTPMostSignificant = sec;
  }
  void WithNtpFrac(uint32_t frac) {
    sr_.NTPLeastSignificant = frac;
  }
  void WithRtpTimestamp(uint32_t rtp_timestamp) {
    sr_.RTPTimestamp = rtp_timestamp;
  }
  void WithPacketCount(uint32_t packet_count) {
    sr_.SenderPacketCount = packet_count;
  }
  void WithOctetCount(uint32_t octet_count) {
    sr_.SenderOctetCount = octet_count;
  }
  void WithReportBlock(ReportBlock* block);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfReportBlocks = 0x1f };

  size_t BlockLength() const {
    const size_t kSrHeaderLength = 8;
    const size_t kSenderInfoLength = 20;
    return kSrHeaderLength + kSenderInfoLength +
           report_blocks_.size() * kReportBlockLength;
  }

  RTCPUtility::RTCPPacketSR sr_;
  std::vector<RTCPUtility::RTCPPacketReportBlockItem> report_blocks_;
};

//
// RTCP receiver report (RFC 3550).
//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P|    RC   |   PT=RR=201   |             length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                     SSRC of packet sender                     |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |                         report block(s)                       |
//  |                            ....                               |

class ReceiverReport : public RtcpPacket {
 public:
  ReceiverReport() : RtcpPacket() {
    memset(&rr_, 0, sizeof(rr_));
  }

  virtual ~ReceiverReport() {}

  void From(uint32_t ssrc) {
    rr_.SenderSSRC = ssrc;
  }
  void WithReportBlock(ReportBlock* block);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfReportBlocks = 0x1f };

  size_t BlockLength() const {
    const size_t kRrHeaderLength = 8;
    return kRrHeaderLength + report_blocks_.size() * kReportBlockLength;
  }

  RTCPUtility::RTCPPacketRR rr_;
  std::vector<RTCPUtility::RTCPPacketReportBlockItem> report_blocks_;
};

// Transmission Time Offsets in RTP Streams (RFC 5450).
//
//      0                   1                   2                   3
//      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// hdr |V=2|P|    RC   |   PT=IJ=195   |             length            |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |                      inter-arrival jitter                     |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     .                                                               .
//     .                                                               .
//     .                                                               .
//     |                      inter-arrival jitter                     |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//  If present, this RTCP packet must be placed after a receiver report
//  (inside a compound RTCP packet), and MUST have the same value for RC
//  (reception report count) as the receiver report.

class Ij : public RtcpPacket {
 public:
  Ij() : RtcpPacket() {}

  virtual ~Ij() {}

  void WithJitterItem(uint32_t jitter);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfIjItems = 0x1f };

  size_t BlockLength() const {
    return kHeaderLength + 4 * ij_items_.size();
  }

  std::vector<uint32_t> ij_items_;

  DISALLOW_COPY_AND_ASSIGN(Ij);
};

// Source Description (SDES) (RFC 3550).
//
//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    SC   |  PT=SDES=202  |             length            |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_1                          |
//   1    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_2                          |
//   2    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//
// Canonical End-Point Identifier SDES Item (CNAME)
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |    CNAME=1    |     length    | user and domain name        ...
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class Sdes : public RtcpPacket {
 public:
  Sdes() : RtcpPacket() {}

  virtual ~Sdes() {}

  void WithCName(uint32_t ssrc, std::string cname);

  struct Chunk {
    uint32_t ssrc;
    std::string name;
    int null_octets;
  };

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfChunks = 0x1f };

  size_t BlockLength() const;

  std::vector<Chunk> chunks_;

  DISALLOW_COPY_AND_ASSIGN(Sdes);
};

//
// Bye packet (BYE) (RFC 3550).
//
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |V=2|P|    SC   |   PT=BYE=203  |             length            |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                           SSRC/CSRC                           |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       :                              ...                              :
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// (opt) |     length    |               reason for leaving            ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class Bye : public RtcpPacket {
 public:
  Bye() : RtcpPacket() {
    memset(&bye_, 0, sizeof(bye_));
  }

  virtual ~Bye() {}

  void From(uint32_t ssrc) {
    bye_.SenderSSRC = ssrc;
  }
  void WithCsrc(uint32_t csrc);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfCsrcs = 0x1f - 1 };

  size_t BlockLength() const {
    size_t source_count = 1 + csrcs_.size();
    return kHeaderLength + 4 * source_count;
  }

  RTCPUtility::RTCPPacketBYE bye_;
  std::vector<uint32_t> csrcs_;
};

// Application-Defined packet (APP) (RFC 3550).
//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P| subtype |   PT=APP=204  |             length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                           SSRC/CSRC                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                          name (ASCII)                         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                   application-dependent data                ...
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class App : public RtcpPacket {
 public:
  App()
      : RtcpPacket(),
        ssrc_(0) {
    memset(&app_, 0, sizeof(app_));
  }

  virtual ~App() {}

  void From(uint32_t ssrc) {
    ssrc_ = ssrc;
  }
  void WithSubType(uint8_t subtype) {
    assert(subtype <= 0x1f);
    app_.SubType = subtype;
  }
  void WithName(uint32_t name) {
    app_.Name = name;
  }
  void WithData(const uint8_t* data, uint16_t data_length) {
    assert(data);
    assert(data_length <= kRtcpAppCode_DATA_SIZE);
    assert(data_length % 4 == 0);
    memcpy(app_.Data, data, data_length);
    app_.Size = data_length;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    return 12 + app_.Size;
  }

  uint32_t ssrc_;
  RTCPUtility::RTCPPacketAPP app_;

  DISALLOW_COPY_AND_ASSIGN(App);
};

// RFC 4585: Feedback format.
//
// Common packet format:
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |V=2|P|   FMT   |       PT      |          length               |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                  SSRC of packet sender                        |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                  SSRC of media source                         |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   :            Feedback Control Information (FCI)                 :
//   :

// Picture loss indication (PLI) (RFC 4585).
//
// FCI: no feedback control information.

class Pli : public RtcpPacket {
 public:
  Pli() : RtcpPacket() {
    memset(&pli_, 0, sizeof(pli_));
  }

  virtual ~Pli() {}

  void From(uint32_t ssrc) {
    pli_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    pli_.MediaSSRC = ssrc;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    return kCommonFbFmtLength;
  }

  RTCPUtility::RTCPPacketPSFBPLI pli_;

  DISALLOW_COPY_AND_ASSIGN(Pli);
};

// Slice loss indication (SLI) (RFC 4585).
//
// FCI:
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |            First        |        Number           | PictureID |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class Sli : public RtcpPacket {
 public:
  Sli() : RtcpPacket() {
    memset(&sli_, 0, sizeof(sli_));
    memset(&sli_item_, 0, sizeof(sli_item_));
  }

  virtual ~Sli() {}

  void From(uint32_t ssrc) {
    sli_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    sli_.MediaSSRC = ssrc;
  }
  void WithFirstMb(uint16_t first_mb) {
    assert(first_mb <= 0x1fff);
    sli_item_.FirstMB = first_mb;
  }
  void WithNumberOfMb(uint16_t number_mb) {
    assert(number_mb <= 0x1fff);
    sli_item_.NumberOfMB = number_mb;
  }
  void WithPictureId(uint8_t picture_id) {
    assert(picture_id <= 0x3f);
    sli_item_.PictureId = picture_id;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    const size_t kFciLength = 4;
    return kCommonFbFmtLength + kFciLength;
  }

  RTCPUtility::RTCPPacketPSFBSLI sli_;
  RTCPUtility::RTCPPacketPSFBSLIItem sli_item_;

  DISALLOW_COPY_AND_ASSIGN(Sli);
};

// Generic NACK (RFC 4585).
//
// FCI:
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |            PID                |             BLP               |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class Nack : public RtcpPacket {
 public:
  Nack() : RtcpPacket() {
    memset(&nack_, 0, sizeof(nack_));
  }

  virtual ~Nack() {}

  void From(uint32_t ssrc) {
    nack_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    nack_.MediaSSRC = ssrc;
  }
  void WithList(const uint16_t* nack_list, int length);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    size_t fci_length = 4 * nack_fields_.size();
    return kCommonFbFmtLength + fci_length;
  }

  RTCPUtility::RTCPPacketRTPFBNACK nack_;
  std::vector<RTCPUtility::RTCPPacketRTPFBNACKItem> nack_fields_;

  DISALLOW_COPY_AND_ASSIGN(Nack);
};

// Reference picture selection indication (RPSI) (RFC 4585).
//
// FCI:
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |      PB       |0| Payload Type|    Native RPSI bit string     |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |   defined per codec          ...                | Padding (0) |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class Rpsi : public RtcpPacket {
 public:
  Rpsi()
      : RtcpPacket(),
        padding_bytes_(0) {
    memset(&rpsi_, 0, sizeof(rpsi_));
  }

  virtual ~Rpsi() {}

  void From(uint32_t ssrc) {
    rpsi_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    rpsi_.MediaSSRC = ssrc;
  }
  void WithPayloadType(uint8_t payload) {
    assert(payload <= 0x7f);
    rpsi_.PayloadType = payload;
  }
  void WithPictureId(uint64_t picture_id);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    size_t fci_length = 2 + (rpsi_.NumberOfValidBits / 8) + padding_bytes_;
    return kCommonFbFmtLength + fci_length;
  }

  uint8_t padding_bytes_;
  RTCPUtility::RTCPPacketPSFBRPSI rpsi_;

  DISALLOW_COPY_AND_ASSIGN(Rpsi);
};

// Full intra request (FIR) (RFC 5104).
//
// FCI:
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                              SSRC                             |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   | Seq nr.       |    Reserved                                   |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class Fir : public RtcpPacket {
 public:
  Fir()
      : RtcpPacket() {
    memset(&fir_, 0, sizeof(fir_));
    memset(&fir_item_, 0, sizeof(fir_item_));
  }

  virtual ~Fir() {}

  void From(uint32_t ssrc) {
    fir_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    fir_item_.SSRC = ssrc;
  }
  void WithCommandSeqNum(uint8_t seq_num) {
    fir_item_.CommandSequenceNumber = seq_num;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    const size_t kFciLength = 8;
    return kCommonFbFmtLength + kFciLength;
  }

  RTCPUtility::RTCPPacketPSFBFIR fir_;
  RTCPUtility::RTCPPacketPSFBFIRItem fir_item_;
};

// Class holding a RTCP packet.
//
// Takes a built rtcp packet.
//  RawPacket raw_packet(buffer, length);
//
// To access the raw packet:
//  raw_packet.buffer();         - pointer to the raw packet
//  raw_packet.buffer_length();  - the length of the raw packet

class RawPacket {
 public:
  RawPacket(const uint8_t* packet, size_t length) {
    assert(length <= IP_PACKET_SIZE);
    memcpy(buffer_, packet, length);
    buffer_length_ = length;
  }

  const uint8_t* buffer() {
    return buffer_;
  }
  size_t buffer_length() const {
    return buffer_length_;
  }

 private:
  size_t buffer_length_;
  uint8_t buffer_[IP_PACKET_SIZE];
};

}  // namespace rtcp
}  // namespace webrtc
#endif  // WEBRTC_MODULES_RTP_RTCP_RTCP_PACKET_H_
