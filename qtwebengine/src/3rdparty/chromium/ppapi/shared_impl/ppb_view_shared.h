// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_PPB_VIEW_SHARED_H_
#define PPAPI_SHARED_IMPL_PPB_VIEW_SHARED_H_

#include "base/compiler_specific.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/thunk/ppb_view_api.h"

namespace ppapi {

// If you add to this struct, be sure to update the serialization in
// ppapi_messages.h.
struct PPAPI_SHARED_EXPORT ViewData {
  ViewData();
  ~ViewData();

  bool Equals(const ViewData& other) const;

  PP_Rect rect;
  bool is_fullscreen;
  bool is_page_visible;
  PP_Rect clip_rect;
  float device_scale;
  float css_scale;
  PP_Point scroll_offset;
};

class PPAPI_SHARED_EXPORT PPB_View_Shared : public Resource,
                                            public thunk::PPB_View_API {
 public:
  PPB_View_Shared(ResourceObjectType type,
                  PP_Instance instance,
                  const ViewData& data);
  virtual ~PPB_View_Shared();

  // Resource overrides.
  virtual thunk::PPB_View_API* AsPPB_View_API() OVERRIDE;

  // PPB_View_API implementation.
  virtual const ViewData& GetData() const OVERRIDE;
  virtual PP_Bool GetRect(PP_Rect* viewport) const OVERRIDE;
  virtual PP_Bool IsFullscreen() const OVERRIDE;
  virtual PP_Bool IsVisible() const OVERRIDE;
  virtual PP_Bool IsPageVisible() const OVERRIDE;
  virtual PP_Bool GetClipRect(PP_Rect* clip) const OVERRIDE;
  virtual float GetDeviceScale() const OVERRIDE;
  virtual float GetCSSScale() const OVERRIDE;
  virtual PP_Bool GetScrollOffset(PP_Point* scroll_offset) const OVERRIDE;

 private:
  ViewData data_;

  DISALLOW_COPY_AND_ASSIGN(PPB_View_Shared);
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_PPB_VIEW_SHARED_H_
