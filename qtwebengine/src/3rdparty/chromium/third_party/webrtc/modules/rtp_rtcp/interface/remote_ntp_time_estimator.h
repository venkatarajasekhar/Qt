/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_REMOTE_NTP_TIME_ESTIMATOR_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_REMOTE_NTP_TIME_ESTIMATOR_H_

#include "webrtc/system_wrappers/interface/rtp_to_ntp.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Clock;
class RtpRtcp;
class TimestampExtrapolator;

// RemoteNtpTimeEstimator can be used to estimate a given RTP timestamp's NTP
// time in local timebase.
// Note that it needs to be trained with at least 2 RTCP SR (by calling
// |UpdateRtcpTimestamp|) before it can be used.
class RemoteNtpTimeEstimator {
 public:
  explicit RemoteNtpTimeEstimator(Clock* clock);

  ~RemoteNtpTimeEstimator();

  // Updates the estimator with the timestamp from newly received RTCP SR for
  // |ssrc|. The RTCP SR is read from |rtp_rtcp|.
  bool UpdateRtcpTimestamp(uint32_t ssrc, RtpRtcp* rtp_rtcp);

  // Estimates the NTP timestamp in local timebase from |rtp_timestamp|.
  // Returns the NTP timestamp in ms when success. -1 if failed.
  int64_t Estimate(uint32_t rtp_timestamp);

 private:
  Clock* clock_;
  scoped_ptr<TimestampExtrapolator> ts_extrapolator_;
  RtcpList rtcp_list_;
  DISALLOW_COPY_AND_ASSIGN(RemoteNtpTimeEstimator);
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_RTP_RTCP_INTERFACE_REMOTE_NTP_TIME_ESTIMATOR_H_
