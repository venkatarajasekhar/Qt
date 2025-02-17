/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_INTERFACE_MOCK_MOCK_VCM_CALLBACKS_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_INTERFACE_MOCK_MOCK_VCM_CALLBACKS_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class MockVCMFrameTypeCallback : public VCMFrameTypeCallback {
 public:
  MOCK_METHOD0(RequestKeyFrame, int32_t());
  MOCK_METHOD1(SliceLossIndicationRequest,
               int32_t(const uint64_t pictureId));
};

class MockPacketRequestCallback : public VCMPacketRequestCallback {
 public:
  MOCK_METHOD2(ResendPackets, int32_t(const uint16_t* sequenceNumbers,
                                      uint16_t length));
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CODING_MAIN_INTERFACE_MOCK_MOCK_VCM_CALLBACKS_H_
