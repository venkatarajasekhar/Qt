// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TIME_ZONE_MONITOR_ANDROID_H_
#define CONTENT_BROWSER_TIME_ZONE_MONITOR_ANDROID_H_

#include "content/browser/time_zone_monitor.h"

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/basictypes.h"

namespace content {

class TimeZoneMonitorAndroid : public TimeZoneMonitor {
 public:
  TimeZoneMonitorAndroid();
  virtual ~TimeZoneMonitorAndroid();

  // Must be called at startup.
  static bool Register(JNIEnv* env);

  // Called by the Java implementation when the system time zone changes.
  void TimeZoneChangedFromJava(JNIEnv* env, jobject caller);

 private:
  // Java provider of system time zone change notifications.
  base::android::ScopedJavaGlobalRef<jobject> impl_;

  DISALLOW_COPY_AND_ASSIGN(TimeZoneMonitorAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TIME_ZONE_MONITOR_ANDROID_H_
