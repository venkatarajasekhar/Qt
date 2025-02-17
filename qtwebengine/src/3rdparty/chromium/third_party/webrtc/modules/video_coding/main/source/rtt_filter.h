/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_
#define WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_

#include "webrtc/typedefs.h"

namespace webrtc
{

class VCMRttFilter
{
public:
    VCMRttFilter();

    VCMRttFilter& operator=(const VCMRttFilter& rhs);

    // Resets the filter.
    void Reset();
    // Updates the filter with a new sample.
    void Update(uint32_t rttMs);
    // A getter function for the current RTT level in ms.
    uint32_t RttMs() const;

private:
    // The size of the drift and jump memory buffers
    // and thus also the detection threshold for these
    // detectors in number of samples.
    enum { kMaxDriftJumpCount = 5 };
    // Detects RTT jumps by comparing the difference between
    // samples and average to the standard deviation.
    // Returns true if the long time statistics should be updated
    // and false otherwise
    bool JumpDetection(uint32_t rttMs);
    // Detects RTT drifts by comparing the difference between
    // max and average to the standard deviation.
    // Returns true if the long time statistics should be updated
    // and false otherwise
    bool DriftDetection(uint32_t rttMs);
    // Computes the short time average and maximum of the vector buf.
    void ShortRttFilter(uint32_t* buf, uint32_t length);

    bool                  _gotNonZeroUpdate;
    double                _avgRtt;
    double                _varRtt;
    uint32_t        _maxRtt;
    uint32_t        _filtFactCount;
    const uint32_t  _filtFactMax;
    const double          _jumpStdDevs;
    const double          _driftStdDevs;
    int32_t         _jumpCount;
    int32_t         _driftCount;
    const int32_t   _detectThreshold;
    uint32_t        _jumpBuf[kMaxDriftJumpCount];
    uint32_t        _driftBuf[kMaxDriftJumpCount];
};

}  // namespace webrtc

#endif // WEBRTC_MODULES_VIDEO_CODING_RTT_FILTER_H_
