// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_VIDEO_DECODER_API_H_
#define PPAPI_THUNK_PPB_VIDEO_DECODER_API_H_

#include "ppapi/c/pp_codecs.h"
#include "ppapi/c/ppb_video_decoder.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_VideoDecoder_API {
 public:
  virtual ~PPB_VideoDecoder_API() {}

  virtual int32_t Initialize(PP_Resource context,
                             PP_VideoProfile profile,
                             PP_Bool allow_software_fallback,
                             scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t Decode(uint32_t decode_id,
                         uint32_t size,
                         const void* buffer,
                         scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t GetPicture(PP_VideoPicture* picture,
                             scoped_refptr<TrackedCallback> callback) = 0;
  virtual void RecyclePicture(const PP_VideoPicture* picture) = 0;
  virtual int32_t Flush(scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t Reset(scoped_refptr<TrackedCallback> callback) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_VIDEO_DECODER_API_H_
