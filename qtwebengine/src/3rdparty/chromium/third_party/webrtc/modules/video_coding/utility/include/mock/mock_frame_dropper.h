/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef WEBRTC_MODULES_VIDEO_CODING_UTILITY_INCLUDE_MOCK_MOCK_FRAME_DROPPER_H_
#define WEBRTC_MODULES_VIDEO_CODING_UTILITY_INCLUDE_MOCK_MOCK_FRAME_DROPPER_H_

#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/video_coding/utility/include/frame_dropper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class MockFrameDropper : public FrameDropper {
 public:
  MOCK_METHOD0(Reset,
      void());
  MOCK_METHOD1(Enable,
      void(bool enable));
  MOCK_METHOD0(DropFrame,
      bool());
  MOCK_METHOD2(Fill,
      void(uint32_t frameSizeBytes, bool deltaFrame));
  MOCK_METHOD1(Leak,
      void(uint32_t inputFrameRate));
  MOCK_METHOD2(SetRates,
      void(float bitRate, float incoming_frame_rate));
  MOCK_CONST_METHOD1(ActualFrameRate,
      float(uint32_t inputFrameRate));
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CODING_UTILITY_INCLUDE_MOCK_MOCK_FRAME_DROPPER_H_
