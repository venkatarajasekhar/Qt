/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import <AVFoundation/AVFoundation.h>

#import "webrtc/modules/video_capture/ios/device_info_ios_objc.h"

@implementation DeviceInfoIosObjC

+ (int)captureDeviceCount {
  return [[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo] count];
}

+ (AVCaptureDevice*)captureDeviceForIndex:(int)index {
  return [[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]
      objectAtIndex:index];
}

+ (AVCaptureDevice*)captureDeviceForUniqueId:(NSString*)uniqueId {
  for (AVCaptureDevice* device in
       [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) {
    if ([uniqueId isEqual:device.uniqueID]) {
      return device;
    }
  }

  return nil;
}

+ (NSString*)deviceNameForIndex:(int)index {
  return [DeviceInfoIosObjC captureDeviceForIndex:index].localizedName;
}

+ (NSString*)deviceUniqueIdForIndex:(int)index {
  return [DeviceInfoIosObjC captureDeviceForIndex:index].uniqueID;
}

+ (NSString*)deviceNameForUniqueId:(NSString*)uniqueId {
  return [[AVCaptureDevice deviceWithUniqueID:uniqueId] localizedName];
}

@end
