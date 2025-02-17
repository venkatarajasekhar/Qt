// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_IMAGE_DATA_API_H_
#define PPAPI_THUNK_PPB_IMAGE_DATA_API_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/ppb_image_data.h"

class SkCanvas;

namespace ppapi {
namespace thunk {

class PPB_ImageData_API {
 public:
  virtual ~PPB_ImageData_API() {}

  virtual PP_Bool Describe(PP_ImageDataDesc* desc) = 0;
  virtual void* Map() = 0;
  virtual void Unmap() = 0;

  // Trusted inteface.
  virtual int32_t GetSharedMemory(int* handle, uint32_t* byte_count) = 0;

  // Get the platform-specific canvas that backs this ImageData, if there is
  // one.
  // The canvas will be NULL:
  //   * If the image is not mapped.
  //   * Within untrusted code (which does not have skia).
  //   * If the ImageData is not backed by a platform-specific image buffer.
  //     This will be the case for ImageDatas created for use in NaCl.
  // For this last reason, you should prefer GetCanvas any time you don't need
  // a platform-specific canvas (e.g., for use with platform-specific APIs).
  // Anything that relies on having a PlatformCanvas will not work for ImageDat
  // objects created from NaCl.
  virtual SkCanvas* GetPlatformCanvas() = 0;

  // Get the canvas that backs this ImageData, if there is one.
  // The canvas will be NULL:
  //   * If the image is not mapped.
  //   * Within untrusted code (which does not have skia).
  virtual SkCanvas* GetCanvas() = 0;

  // Signal that this image is a good candidate for reuse. Call this from APIs
  // that receive ImageData resources of a fixed size and where the plugin will
  // release its reference to the ImageData. If the current implementation
  // supports image data reuse (only supported out-of-process) then the
  // ImageData will be marked and potentially cached for re-use.
  virtual void SetIsCandidateForReuse() = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_IMAGE_DATA_API_H_
