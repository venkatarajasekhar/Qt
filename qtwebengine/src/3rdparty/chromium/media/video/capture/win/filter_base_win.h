// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implement a simple base class for DirectShow filters. It may only be used in
// a single threaded apartment.

#ifndef MEDIA_VIDEO_CAPTURE_WIN_FILTER_BASE_WIN_H_
#define MEDIA_VIDEO_CAPTURE_WIN_FILTER_BASE_WIN_H_

// Avoid including strsafe.h via dshow as it will cause build warnings.
#define NO_DSHOW_STRSAFE
#include <dshow.h>

#include "base/memory/ref_counted.h"
#include "base/win/scoped_comptr.h"

namespace media {

class FilterBase
    : public IBaseFilter,
      public base::RefCounted<FilterBase> {
 public:
  FilterBase();
  virtual ~FilterBase();

  // Number of pins connected to this filter.
  virtual size_t NoOfPins() = 0;
  // Returns the IPin interface pin no index.
  virtual IPin* GetPin(int index) = 0;

  // Inherited from IUnknown.
  STDMETHOD(QueryInterface)(REFIID id, void** object_ptr);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // Inherited from IBaseFilter.
  STDMETHOD(EnumPins)(IEnumPins** enum_pins);

  STDMETHOD(FindPin)(LPCWSTR id, IPin** pin);

  STDMETHOD(QueryFilterInfo)(FILTER_INFO* info);

  STDMETHOD(JoinFilterGraph)(IFilterGraph* graph, LPCWSTR name);

  STDMETHOD(QueryVendorInfo)(LPWSTR* vendor_info);

  // Inherited from IMediaFilter.
  STDMETHOD(Stop)();

  STDMETHOD(Pause)();

  STDMETHOD(Run)(REFERENCE_TIME start);

  STDMETHOD(GetState)(DWORD msec_timeout, FILTER_STATE* state);

  STDMETHOD(SetSyncSource)(IReferenceClock* clock);

  STDMETHOD(GetSyncSource)(IReferenceClock** clock);

  // Inherited from IPersistent.
  STDMETHOD(GetClassID)(CLSID* class_id) = 0;

 private:
  FILTER_STATE state_;
  base::win::ScopedComPtr<IFilterGraph> owning_graph_;

  DISALLOW_COPY_AND_ASSIGN(FilterBase);
};

}  // namespace media

#endif  // MEDIA_VIDEO_CAPTURE_WIN_FILTER_BASE_WIN_H_
