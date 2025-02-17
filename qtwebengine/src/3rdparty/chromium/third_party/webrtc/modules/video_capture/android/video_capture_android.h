/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_VIDEO_CAPTURE_ANDROID_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_VIDEO_CAPTURE_ANDROID_H_

#include <jni.h>

#include "webrtc/modules/video_capture/android/device_info_android.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"

namespace webrtc {
namespace videocapturemodule {

class VideoCaptureAndroid : public VideoCaptureImpl {
 public:
  VideoCaptureAndroid(const int32_t id);
  virtual int32_t Init(const int32_t id, const char* deviceUniqueIdUTF8);

  virtual int32_t StartCapture(const VideoCaptureCapability& capability);
  virtual int32_t StopCapture();
  virtual bool CaptureStarted();
  virtual int32_t CaptureSettings(VideoCaptureCapability& settings);
  virtual int32_t SetCaptureRotation(VideoCaptureRotation rotation);

  int32_t OnIncomingFrame(uint8_t* videoFrame,
                          int32_t videoFrameLength,
                          int64_t captureTime = 0);

 protected:
  virtual ~VideoCaptureAndroid();

  DeviceInfoAndroid _deviceInfo;
  jobject _jCapturer; // Global ref to Java VideoCaptureAndroid object.
  VideoCaptureCapability _captureCapability;
  bool _captureStarted;
};

}  // namespace videocapturemodule
}  // namespace webrtc
#endif // WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_VIDEO_CAPTURE_ANDROID_H_
