/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_RECEIVER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_RECEIVER_H_

#include <map>
#include <vector>
#include <set>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtcp_receiver_help.h"
#include "webrtc/modules/rtp_rtcp/source/rtcp_utility.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/modules/rtp_rtcp/source/tmmbr_help.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class ModuleRtpRtcpImpl;

class RTCPReceiver : public TMMBRHelp
{
public:
    RTCPReceiver(const int32_t id, Clock* clock,
                 ModuleRtpRtcpImpl* owner);
    virtual ~RTCPReceiver();

    void ChangeUniqueId(const int32_t id);

    RTCPMethod Status() const;
    int32_t SetRTCPStatus(const RTCPMethod method);

    int64_t LastReceived();
    int64_t LastReceivedReceiverReport() const;

    void SetSsrcs(uint32_t main_ssrc,
                  const std::set<uint32_t>& registered_ssrcs);
    void SetRelaySSRC( const uint32_t ssrc);
    int32_t SetRemoteSSRC( const uint32_t ssrc);
    uint32_t RemoteSSRC() const;

    uint32_t RelaySSRC() const;

    void RegisterRtcpObservers(RtcpIntraFrameObserver* intra_frame_callback,
                               RtcpBandwidthObserver* bandwidth_callback,
                               RtcpFeedback* feedback_callback);

    int32_t IncomingRTCPPacket(
        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation,
        RTCPUtility::RTCPParserV2 *rtcpParser);

    void TriggerCallbacksFromRTCPPacket(RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    // get received cname
    int32_t CNAME(const uint32_t remoteSSRC,
                  char cName[RTCP_CNAME_SIZE]) const;

    // get received NTP
    int32_t NTP(uint32_t *ReceivedNTPsecs,
                uint32_t *ReceivedNTPfrac,
                uint32_t *RTCPArrivalTimeSecs,
                uint32_t *RTCPArrivalTimeFrac,
                uint32_t *rtcp_timestamp) const;

   bool LastReceivedXrReferenceTimeInfo(RtcpReceiveTimeInfo* info) const;

    // get rtt
    int32_t RTT(uint32_t remoteSSRC,
                uint16_t* RTT,
                uint16_t* avgRTT,
                uint16_t* minRTT,
                uint16_t* maxRTT) const;

    int32_t ResetRTT(const uint32_t remoteSSRC);

    int32_t SenderInfoReceived(RTCPSenderInfo* senderInfo) const;

    bool GetAndResetXrRrRtt(uint16_t* rtt_ms);

    // get statistics
    int32_t StatisticsReceived(
        std::vector<RTCPReportBlock>* receiveBlocks) const;

    void GetPacketTypeCounter(RtcpPacketTypeCounter* packet_counter) const;

    // Returns true if we haven't received an RTCP RR for several RTCP
    // intervals, but only triggers true once.
    bool RtcpRrTimeout(int64_t rtcp_interval_ms);

    // Returns true if we haven't received an RTCP RR telling the receive side
    // has not received RTP packets for too long, i.e. extended highest sequence
    // number hasn't increased for several RTCP intervals. The function only
    // returns true once until a new RR is received.
    bool RtcpRrSequenceNumberTimeout(int64_t rtcp_interval_ms);

    // Get TMMBR
    int32_t TMMBRReceived(const uint32_t size,
                          const uint32_t accNumCandidates,
                          TMMBRSet* candidateSet) const;

    bool UpdateRTCPReceiveInformationTimers();

    int32_t BoundingSet(bool &tmmbrOwner, TMMBRSet* boundingSetRec);

    int32_t UpdateTMMBR();

    void RegisterRtcpStatisticsCallback(RtcpStatisticsCallback* callback);
    RtcpStatisticsCallback* GetRtcpStatisticsCallback();

protected:
    RTCPHelp::RTCPReportBlockInformation* CreateReportBlockInformation(const uint32_t remoteSSRC);
    RTCPHelp::RTCPReportBlockInformation* GetReportBlockInformation(const uint32_t remoteSSRC) const;

    RTCPUtility::RTCPCnameInformation* CreateCnameInformation(const uint32_t remoteSSRC);
    RTCPUtility::RTCPCnameInformation* GetCnameInformation(const uint32_t remoteSSRC) const;

    RTCPHelp::RTCPReceiveInformation* CreateReceiveInformation(const uint32_t remoteSSRC);
    RTCPHelp::RTCPReceiveInformation* GetReceiveInformation(const uint32_t remoteSSRC);

    void UpdateReceiveInformation( RTCPHelp::RTCPReceiveInformation& receiveInformation);

    void HandleSenderReceiverReport(RTCPUtility::RTCPParserV2& rtcpParser,
                                    RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleReportBlock(const RTCPUtility::RTCPPacket& rtcpPacket,
                           RTCPHelp::RTCPPacketInformation& rtcpPacketInformation,
                           const uint32_t remoteSSRC,
                           const uint8_t numberOfReportBlocks);

    void HandleSDES(RTCPUtility::RTCPParserV2& rtcpParser);

    void HandleSDESChunk(RTCPUtility::RTCPParserV2& rtcpParser);

    void HandleXrHeader(RTCPUtility::RTCPParserV2& parser,
                        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleXrReceiveReferenceTime(
        RTCPUtility::RTCPParserV2& parser,
        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleXrDlrrReportBlock(
        RTCPUtility::RTCPParserV2& parser,
        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleXrDlrrReportBlockItem(
        const RTCPUtility::RTCPPacket& packet,
        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleXRVOIPMetric(RTCPUtility::RTCPParserV2& rtcpParser,
                            RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleNACK(RTCPUtility::RTCPParserV2& rtcpParser,
                    RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleNACKItem(const RTCPUtility::RTCPPacket& rtcpPacket,
                        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleBYE(RTCPUtility::RTCPParserV2& rtcpParser);

    void HandlePLI(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleSLI(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleSLIItem(const RTCPUtility::RTCPPacket& rtcpPacket,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleRPSI(RTCPUtility::RTCPParserV2& rtcpParser,
                    RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandlePsfbApp(RTCPUtility::RTCPParserV2& rtcpParser,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleREMBItem(RTCPUtility::RTCPParserV2& rtcpParser,
                        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleIJ(RTCPUtility::RTCPParserV2& rtcpParser,
                  RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleIJItem(const RTCPUtility::RTCPPacket& rtcpPacket,
                      RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleTMMBR(RTCPUtility::RTCPParserV2& rtcpParser,
                     RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleTMMBRItem(RTCPHelp::RTCPReceiveInformation& receiveInfo,
                         const RTCPUtility::RTCPPacket& rtcpPacket,
                         RTCPHelp::RTCPPacketInformation& rtcpPacketInformation,
                         const uint32_t senderSSRC);

    void HandleTMMBN(RTCPUtility::RTCPParserV2& rtcpParser,
                     RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleSR_REQ(RTCPUtility::RTCPParserV2& rtcpParser,
                      RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleTMMBNItem(RTCPHelp::RTCPReceiveInformation& receiveInfo,
                         const RTCPUtility::RTCPPacket& rtcpPacket);

    void HandleFIR(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleFIRItem(RTCPHelp::RTCPReceiveInformation* receiveInfo,
                       const RTCPUtility::RTCPPacket& rtcpPacket,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleAPP(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleAPPItem(RTCPUtility::RTCPParserV2& rtcpParser,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

 private:
  typedef std::map<uint32_t, RTCPHelp::RTCPReceiveInformation*>
      ReceivedInfoMap;
  int32_t           _id;
  Clock*                  _clock;
  RTCPMethod              _method;
  int64_t           _lastReceived;
  ModuleRtpRtcpImpl&      _rtpRtcp;

  CriticalSectionWrapper* _criticalSectionFeedbacks;
  RtcpFeedback*           _cbRtcpFeedback;
  RtcpBandwidthObserver*  _cbRtcpBandwidthObserver;
  RtcpIntraFrameObserver* _cbRtcpIntraFrameObserver;

  CriticalSectionWrapper* _criticalSectionRTCPReceiver;
  uint32_t          main_ssrc_;
  uint32_t          _remoteSSRC;
  std::set<uint32_t> registered_ssrcs_;

  // Received send report
  RTCPSenderInfo _remoteSenderInfo;
  // when did we receive the last send report
  uint32_t _lastReceivedSRNTPsecs;
  uint32_t _lastReceivedSRNTPfrac;

  // Received XR receive time report.
  RtcpReceiveTimeInfo _remoteXRReceiveTimeInfo;
  // Time when the report was received.
  uint32_t _lastReceivedXRNTPsecs;
  uint32_t _lastReceivedXRNTPfrac;
  // Estimated rtt, zero when there is no valid estimate.
  uint16_t xr_rr_rtt_ms_;

  // Received report blocks.
  std::map<uint32_t, RTCPHelp::RTCPReportBlockInformation*>
      _receivedReportBlockMap;
  ReceivedInfoMap _receivedInfoMap;
  std::map<uint32_t, RTCPUtility::RTCPCnameInformation*>
      _receivedCnameMap;

  uint32_t            _packetTimeOutMS;

  // The last time we received an RTCP RR.
  int64_t _lastReceivedRrMs;

  // The time we last received an RTCP RR telling we have ssuccessfully
  // delivered RTP packet to the remote side.
  int64_t _lastIncreasedSequenceNumberMs;

  RtcpStatisticsCallback* stats_callback_;

  RtcpPacketTypeCounter packet_type_counter_;
};
}  // namespace webrtc
#endif // WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_RECEIVER_H_
