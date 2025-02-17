/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/file_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/voice_engine/include/voe_errors.h"
#include "webrtc/voice_engine/voe_rtp_rtcp_impl.h"
#include "webrtc/voice_engine/voice_engine_impl.h"

#include "webrtc/voice_engine/channel.h"
#include "webrtc/voice_engine/transmit_mixer.h"

namespace webrtc {

VoERTP_RTCP* VoERTP_RTCP::GetInterface(VoiceEngine* voiceEngine)
{
#ifndef WEBRTC_VOICE_ENGINE_RTP_RTCP_API
    return NULL;
#else
    if (NULL == voiceEngine)
    {
        return NULL;
    }
    VoiceEngineImpl* s = static_cast<VoiceEngineImpl*>(voiceEngine);
    s->AddRef();
    return s;
#endif
}

#ifdef WEBRTC_VOICE_ENGINE_RTP_RTCP_API

VoERTP_RTCPImpl::VoERTP_RTCPImpl(voe::SharedData* shared) : _shared(shared)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoERTP_RTCPImpl::VoERTP_RTCPImpl() - ctor");
}

VoERTP_RTCPImpl::~VoERTP_RTCPImpl()
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoERTP_RTCPImpl::~VoERTP_RTCPImpl() - dtor");
}

int VoERTP_RTCPImpl::SetLocalSSRC(int channel, unsigned int ssrc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetLocalSSRC(channel=%d, %lu)", channel, ssrc);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetLocalSSRC() failed to locate channel");
        return -1;
    }
    return channelPtr->SetLocalSSRC(ssrc);
}

int VoERTP_RTCPImpl::GetLocalSSRC(int channel, unsigned int& ssrc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetLocalSSRC(channel=%d, ssrc=?)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetLocalSSRC() failed to locate channel");
        return -1;
    }
    return channelPtr->GetLocalSSRC(ssrc);
}

int VoERTP_RTCPImpl::GetRemoteSSRC(int channel, unsigned int& ssrc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRemoteSSRC(channel=%d, ssrc=?)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRemoteSSRC() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRemoteSSRC(ssrc);
}

int VoERTP_RTCPImpl::SetSendAudioLevelIndicationStatus(int channel,
                                                       bool enable,
                                                       unsigned char id)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetSendAudioLevelIndicationStatus(channel=%d, enable=%d,"
                 " ID=%u)", channel, enable, id);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    if (enable && (id < kVoiceEngineMinRtpExtensionId ||
                   id > kVoiceEngineMaxRtpExtensionId))
    {
        // [RFC5285] The 4-bit id is the local identifier of this element in
        // the range 1-14 inclusive.
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
            "SetSendAudioLevelIndicationStatus() invalid ID parameter");
        return -1;
    }

    // Set state and id for the specified channel.
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetSendAudioLevelIndicationStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->SetSendAudioLevelIndicationStatus(enable, id);
}

int VoERTP_RTCPImpl::SetReceiveAudioLevelIndicationStatus(int channel,
                                                          bool enable,
                                                          unsigned char id) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
      "SetReceiveAudioLevelIndicationStatus(channel=%d, enable=%d, id=%u)",
      channel, enable, id);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return -1;
  }
  if (enable &&
      (id < kVoiceEngineMinRtpExtensionId ||
       id > kVoiceEngineMaxRtpExtensionId)) {
    // [RFC5285] The 4-bit id is the local identifier of this element in
    // the range 1-14 inclusive.
    _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
        "SetReceiveAbsoluteSenderTimeStatus() invalid id parameter");
    return -1;
  }
  // Set state and id for the specified channel.
  voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
  voe::Channel* channel_ptr = ch.channel();
  if (channel_ptr == NULL) {
    _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
        "SetReceiveAudioLevelIndicationStatus() failed to locate channel");
    return -1;
  }
  return channel_ptr->SetReceiveAudioLevelIndicationStatus(enable, id);
}

int VoERTP_RTCPImpl::SetSendAbsoluteSenderTimeStatus(int channel,
                                                     bool enable,
                                                     unsigned char id) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "SetSendAbsoluteSenderTimeStatus(channel=%d, enable=%d, id=%u)",
               channel, enable, id);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return -1;
  }
  if (enable && (id < kVoiceEngineMinRtpExtensionId ||
                 id > kVoiceEngineMaxRtpExtensionId)) {
    // [RFC5285] The 4-bit id is the local identifier of this element in
    // the range 1-14 inclusive.
    _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
        "SetSendAbsoluteSenderTimeStatus() invalid id parameter");
    return -1;
  }
  // Set state and id for the specified channel.
  voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
  voe::Channel* channelPtr = ch.channel();
  if (channelPtr == NULL) {
    _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
        "SetSendAbsoluteSenderTimeStatus() failed to locate channel");
    return -1;
  }
  return channelPtr->SetSendAbsoluteSenderTimeStatus(enable, id);
}

int VoERTP_RTCPImpl::SetReceiveAbsoluteSenderTimeStatus(int channel,
                                                        bool enable,
                                                        unsigned char id) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
      "SetReceiveAbsoluteSenderTimeStatus(channel=%d, enable=%d, id=%u)",
      channel, enable, id);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return -1;
  }
  if (enable && (id < kVoiceEngineMinRtpExtensionId ||
                 id > kVoiceEngineMaxRtpExtensionId)) {
    // [RFC5285] The 4-bit id is the local identifier of this element in
    // the range 1-14 inclusive.
    _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
        "SetReceiveAbsoluteSenderTimeStatus() invalid id parameter");
    return -1;
  }
  // Set state and id for the specified channel.
  voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
  voe::Channel* channelPtr = ch.channel();
  if (channelPtr == NULL) {
    _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
        "SetReceiveAbsoluteSenderTimeStatus() failed to locate channel");
    return -1;
  }
  return channelPtr->SetReceiveAbsoluteSenderTimeStatus(enable, id);
}

int VoERTP_RTCPImpl::SetRTCPStatus(int channel, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetRTCPStatus(channel=%d, enable=%d)", channel, enable);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetRTCPStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->SetRTCPStatus(enable);
}

int VoERTP_RTCPImpl::GetRTCPStatus(int channel, bool& enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRTCPStatus(channel=%d)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRTCPStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRTCPStatus(enabled);
}

int VoERTP_RTCPImpl::SetRTCP_CNAME(int channel, const char cName[256])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetRTCP_CNAME(channel=%d, cName=%s)", channel, cName);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetRTCP_CNAME() failed to locate channel");
        return -1;
    }
    return channelPtr->SetRTCP_CNAME(cName);
}

int VoERTP_RTCPImpl::GetRTCP_CNAME(int channel, char cName[256])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRTCP_CNAME(channel=%d, cName=?)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRTCP_CNAME() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRTCP_CNAME(cName);
}

int VoERTP_RTCPImpl::GetRemoteRTCP_CNAME(int channel, char cName[256])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRemoteRTCP_CNAME(channel=%d, cName=?)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRemoteRTCP_CNAME() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRemoteRTCP_CNAME(cName);
}

int VoERTP_RTCPImpl::GetRemoteRTCPData(
    int channel,
    unsigned int& NTPHigh, // from sender info in SR
    unsigned int& NTPLow, // from sender info in SR
    unsigned int& timestamp, // from sender info in SR
    unsigned int& playoutTimestamp, // derived locally
    unsigned int* jitter, // from report block 1 in SR/RR
    unsigned short* fractionLost) // from report block 1 in SR/RR
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRemoteRTCPData(channel=%d,...)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRemoteRTCP_CNAME() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRemoteRTCPData(NTPHigh,
                                         NTPLow,
                                         timestamp,
                                         playoutTimestamp,
                                         jitter,
                                         fractionLost);
}

int VoERTP_RTCPImpl::GetRTPStatistics(int channel,
                                      unsigned int& averageJitterMs,
                                      unsigned int& maxJitterMs,
                                      unsigned int& discardedPackets)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRTPStatistics(channel=%d,....)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRTPStatistics() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRTPStatistics(averageJitterMs,
                                        maxJitterMs,
                                        discardedPackets);
}

int VoERTP_RTCPImpl::GetRTCPStatistics(int channel, CallStatistics& stats)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRTCPStatistics(channel=%d)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetRTPStatistics() failed to locate channel");
        return -1;
    }
    return channelPtr->GetRTPStatistics(stats);
}

int VoERTP_RTCPImpl::GetRemoteRTCPReportBlocks(
    int channel, std::vector<ReportBlock>* report_blocks) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "GetRemoteRTCPReportBlocks(channel=%d)", channel);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return -1;
  }
  voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
  voe::Channel* channel_ptr = ch.channel();
  if (channel_ptr == NULL) {
    _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
        "GetRemoteRTCPReportBlocks() failed to locate channel");
    return -1;
  }
  return channel_ptr->GetRemoteRTCPReportBlocks(report_blocks);
}

int VoERTP_RTCPImpl::SetREDStatus(int channel, bool enable, int redPayloadtype)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetREDStatus(channel=%d, enable=%d, redPayloadtype=%d)",
                 channel, enable, redPayloadtype);
#ifdef WEBRTC_CODEC_RED
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetREDStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->SetREDStatus(enable, redPayloadtype);
#else
    _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
        "SetREDStatus() RED is not supported");
    return -1;
#endif
}

int VoERTP_RTCPImpl::GetREDStatus(int channel,
                                  bool& enabled,
                                  int& redPayloadtype)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetREDStatus(channel=%d, enabled=?, redPayloadtype=?)",
                 channel);
#ifdef WEBRTC_CODEC_RED
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetREDStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->GetREDStatus(enabled, redPayloadtype);
#else
    _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
        "GetREDStatus() RED is not supported");
    return -1;
#endif
}

int VoERTP_RTCPImpl::SetNACKStatus(int channel,
                                   bool enable,
                                   int maxNoPackets)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetNACKStatus(channel=%d, enable=%d, maxNoPackets=%d)",
                 channel, enable, maxNoPackets);

    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetNACKStatus() failed to locate channel");
        return -1;
    }
    channelPtr->SetNACKStatus(enable, maxNoPackets);
    return 0;
}


int VoERTP_RTCPImpl::StartRTPDump(int channel,
                                  const char fileNameUTF8[1024],
                                  RTPDirections direction)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StartRTPDump(channel=%d, fileNameUTF8=%s, direction=%d)",
                 channel, fileNameUTF8, direction);
    assert(1024 == FileWrapper::kMaxFileNameSize);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StartRTPDump() failed to locate channel");
        return -1;
    }
    return channelPtr->StartRTPDump(fileNameUTF8, direction);
}

int VoERTP_RTCPImpl::StopRTPDump(int channel, RTPDirections direction)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StopRTPDump(channel=%d, direction=%d)", channel, direction);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StopRTPDump() failed to locate channel");
        return -1;
    }
    return channelPtr->StopRTPDump(direction);
}

int VoERTP_RTCPImpl::RTPDumpIsActive(int channel, RTPDirections direction)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "RTPDumpIsActive(channel=%d, direction=%d)",
                 channel, direction);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StopRTPDump() failed to locate channel");
        return -1;
    }
    return channelPtr->RTPDumpIsActive(direction);
}

int VoERTP_RTCPImpl::SetVideoEngineBWETarget(int channel,
                                             ViENetwork* vie_network,
                                             int video_channel) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
      "SetVideoEngineBWETarget(channel=%d, vie_network=?, video_channel=%d)",
      channel, vie_network, video_channel);

  voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
  voe::Channel* channelPtr = ch.channel();
  if (channelPtr == NULL) {
    _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
                          "SetVideoEngineBWETarget() failed to locate channel");
    if (vie_network) {
      vie_network->Release();
    }
    return -1;
  }
  channelPtr->SetVideoEngineBWETarget(vie_network, video_channel);
  return 0;
}

#endif  // #ifdef WEBRTC_VOICE_ENGINE_RTP_RTCP_API

}  // namespace webrtc
