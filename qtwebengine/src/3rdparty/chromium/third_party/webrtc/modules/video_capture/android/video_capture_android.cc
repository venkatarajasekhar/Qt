/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/video_capture/android/video_capture_android.h"

#include "webrtc/base/common.h"
#include "webrtc/modules/utility/interface/helpers_android.h"
#include "webrtc/modules/video_capture/android/device_info_android.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/logcat_trace_context.h"
#include "webrtc/system_wrappers/interface/ref_count.h"
#include "webrtc/system_wrappers/interface/trace.h"

static JavaVM* g_jvm = NULL;
static jclass g_java_capturer_class = NULL;  // VideoCaptureAndroid.class.
static jobject g_context = NULL;  // Owned android.content.Context.

namespace webrtc {

// Called by Java to get the global application context.
jobject JNICALL GetContext(JNIEnv* env, jclass) {
  assert(g_context);
  return g_context;
}

// Called by Java when the camera has a new frame to deliver.
void JNICALL ProvideCameraFrame(
    JNIEnv* env,
    jobject,
    jbyteArray javaCameraFrame,
    jint length,
    jlong context) {
  webrtc::videocapturemodule::VideoCaptureAndroid* captureModule =
      reinterpret_cast<webrtc::videocapturemodule::VideoCaptureAndroid*>(
          context);
  jbyte* cameraFrame = env->GetByteArrayElements(javaCameraFrame, NULL);
  captureModule->OnIncomingFrame(
      reinterpret_cast<uint8_t*>(cameraFrame), length, 0);
  env->ReleaseByteArrayElements(javaCameraFrame, cameraFrame, JNI_ABORT);
}

// Called by Java when the device orientation has changed.
void JNICALL OnOrientationChanged(
    JNIEnv* env, jobject, jlong context, jint degrees) {
  webrtc::videocapturemodule::VideoCaptureAndroid* captureModule =
      reinterpret_cast<webrtc::videocapturemodule::VideoCaptureAndroid*>(
          context);
  degrees = (360 + degrees) % 360;
  assert(degrees >= 0 && degrees < 360);
  VideoCaptureRotation rotation =
      (degrees <= 45 || degrees > 315) ? kCameraRotate0 :
      (degrees > 45 && degrees <= 135) ? kCameraRotate90 :
      (degrees > 135 && degrees <= 225) ? kCameraRotate180 :
      (degrees > 225 && degrees <= 315) ? kCameraRotate270 :
      kCameraRotate0;  // Impossible.
  int32_t status =
      captureModule->VideoCaptureImpl::SetCaptureRotation(rotation);
  RTC_UNUSED(status);
  assert(status == 0);
}

int32_t SetCaptureAndroidVM(JavaVM* javaVM, jobject context) {
  if (javaVM) {
    assert(!g_jvm);
    g_jvm = javaVM;
    AttachThreadScoped ats(g_jvm);
    g_context = ats.env()->NewGlobalRef(context);

    videocapturemodule::DeviceInfoAndroid::Initialize(ats.env());

    jclass j_capture_class =
        ats.env()->FindClass("org/webrtc/videoengine/VideoCaptureAndroid");
    assert(j_capture_class);
    g_java_capturer_class =
        reinterpret_cast<jclass>(ats.env()->NewGlobalRef(j_capture_class));
    assert(g_java_capturer_class);

    JNINativeMethod native_methods[] = {
        {"GetContext",
         "()Landroid/content/Context;",
         reinterpret_cast<void*>(&GetContext)},
        {"OnOrientationChanged",
         "(JI)V",
         reinterpret_cast<void*>(&OnOrientationChanged)},
        {"ProvideCameraFrame",
         "([BIJ)V",
         reinterpret_cast<void*>(&ProvideCameraFrame)}};
    if (ats.env()->RegisterNatives(g_java_capturer_class,
                                   native_methods, 3) != 0)
      assert(false);
  } else {
    if (g_jvm) {
      AttachThreadScoped ats(g_jvm);
      ats.env()->UnregisterNatives(g_java_capturer_class);
      ats.env()->DeleteGlobalRef(g_java_capturer_class);
      g_java_capturer_class = NULL;
      ats.env()->DeleteGlobalRef(g_context);
      g_context = NULL;
      videocapturemodule::DeviceInfoAndroid::DeInitialize();
      g_jvm = NULL;
    }
  }

  return 0;
}

namespace videocapturemodule {

VideoCaptureModule* VideoCaptureImpl::Create(
    const int32_t id,
    const char* deviceUniqueIdUTF8) {
  RefCountImpl<videocapturemodule::VideoCaptureAndroid>* implementation =
      new RefCountImpl<videocapturemodule::VideoCaptureAndroid>(id);
  if (implementation->Init(id, deviceUniqueIdUTF8) != 0) {
    delete implementation;
    implementation = NULL;
  }
  return implementation;
}

int32_t VideoCaptureAndroid::OnIncomingFrame(uint8_t* videoFrame,
                                             int32_t videoFrameLength,
                                             int64_t captureTime) {
  return IncomingFrame(
      videoFrame, videoFrameLength, _captureCapability, captureTime);
}

VideoCaptureAndroid::VideoCaptureAndroid(const int32_t id)
    : VideoCaptureImpl(id),
      _deviceInfo(id),
      _jCapturer(NULL),
      _captureStarted(false) {
}

int32_t VideoCaptureAndroid::Init(const int32_t id,
                                  const char* deviceUniqueIdUTF8) {
  const int nameLength = strlen(deviceUniqueIdUTF8);
  if (nameLength >= kVideoCaptureUniqueNameLength)
    return -1;

  // Store the device name
  _deviceUniqueId = new char[nameLength + 1];
  memcpy(_deviceUniqueId, deviceUniqueIdUTF8, nameLength + 1);

  AttachThreadScoped ats(g_jvm);
  JNIEnv* env = ats.env();

  jmethodID ctor = env->GetMethodID(g_java_capturer_class, "<init>", "(IJ)V");
  assert(ctor);
  jlong j_this = reinterpret_cast<intptr_t>(this);
  size_t camera_id = 0;
  if (!_deviceInfo.FindCameraIndex(deviceUniqueIdUTF8, &camera_id))
    return -1;
  _jCapturer = env->NewGlobalRef(
      env->NewObject(g_java_capturer_class, ctor, camera_id, j_this));
  assert(_jCapturer);
  return 0;
}

VideoCaptureAndroid::~VideoCaptureAndroid() {
  // Ensure Java camera is released even if our caller didn't explicitly Stop.
  if (_captureStarted)
    StopCapture();
  AttachThreadScoped ats(g_jvm);
  ats.env()->DeleteGlobalRef(_jCapturer);
}

int32_t VideoCaptureAndroid::StartCapture(
    const VideoCaptureCapability& capability) {
  CriticalSectionScoped cs(&_apiCs);
  AttachThreadScoped ats(g_jvm);
  JNIEnv* env = ats.env();

  if (_deviceInfo.GetBestMatchedCapability(
          _deviceUniqueId, capability, _captureCapability) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: GetBestMatchedCapability failed: %dx%d",
                 __FUNCTION__, capability.width, capability.height);
    return -1;
  }

  _captureDelay = _captureCapability.expectedCaptureDelay;

  jmethodID j_start =
      env->GetMethodID(g_java_capturer_class, "startCapture", "(IIII)Z");
  assert(j_start);
  int min_mfps = 0;
  int max_mfps = 0;
  _deviceInfo.GetMFpsRange(_deviceUniqueId, _captureCapability.maxFPS,
                           &min_mfps, &max_mfps);
  bool started = env->CallBooleanMethod(_jCapturer, j_start,
                                        _captureCapability.width,
                                        _captureCapability.height,
                                        min_mfps, max_mfps);
  if (started) {
    _requestedCapability = capability;
    _captureStarted = true;
  }
  return started ? 0 : -1;
}

int32_t VideoCaptureAndroid::StopCapture() {
  CriticalSectionScoped cs(&_apiCs);
  AttachThreadScoped ats(g_jvm);
  JNIEnv* env = ats.env();

  memset(&_requestedCapability, 0, sizeof(_requestedCapability));
  memset(&_captureCapability, 0, sizeof(_captureCapability));
  _captureStarted = false;

  jmethodID j_stop =
      env->GetMethodID(g_java_capturer_class, "stopCapture", "()Z");
  return env->CallBooleanMethod(_jCapturer, j_stop) ? 0 : -1;
}

bool VideoCaptureAndroid::CaptureStarted() {
  CriticalSectionScoped cs(&_apiCs);
  return _captureStarted;
}

int32_t VideoCaptureAndroid::CaptureSettings(
    VideoCaptureCapability& settings) {
  CriticalSectionScoped cs(&_apiCs);
  settings = _requestedCapability;
  return 0;
}

int32_t VideoCaptureAndroid::SetCaptureRotation(
    VideoCaptureRotation rotation) {
  CriticalSectionScoped cs(&_apiCs);
  int32_t status = VideoCaptureImpl::SetCaptureRotation(rotation);
  if (status != 0)
    return status;

  AttachThreadScoped ats(g_jvm);
  JNIEnv* env = ats.env();

  jmethodID j_spr =
      env->GetMethodID(g_java_capturer_class, "setPreviewRotation", "(I)V");
  assert(j_spr);
  int rotation_degrees;
  if (RotationInDegrees(rotation, &rotation_degrees) != 0) {
    assert(false);
  }
  env->CallVoidMethod(_jCapturer, j_spr, rotation_degrees);
  return 0;
}

}  // namespace videocapturemodule
}  // namespace webrtc
