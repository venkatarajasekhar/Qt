// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the definition of VASurface class, used for decoding by
// VaapiVideoDecodeAccelerator and VaapiH264Decoder.

#ifndef CONTENT_COMMON_GPU_MEDIA_VA_SURFACE_H_
#define CONTENT_COMMON_GPU_MEDIA_VA_SURFACE_H_

#include "third_party/libva/va/va.h"

namespace content {

// A VA-API-specific decode surface used by VaapiH264Decoder to decode into
// and use as reference for decoding other surfaces. It is also handed by the
// decoder to VaapiVideoDecodeAccelerator when the contents of the surface are
// ready and should be displayed. VAVDA converts the surface contents into an
// X Pixmap bound to a texture for display and releases its reference to it.
// Decoder releases its references to the surface when it's done decoding and
// using it as reference. Note that a surface may still be used for reference
// after it's been sent to output and also after it is no longer used by VAVDA.
// Thus, the surface can be in use by both VAVDA and the Decoder at the same
// time, or by either of them, with the restriction that VAVDA will never get
// the surface until the contents are ready, and it is guaranteed that the
// contents will not change after that.
// When both the decoder and VAVDA release their references to the surface,
// it is freed and the release callback is executed to put the surface back
// into available surfaces pool, which is managed externally.
//
//   VASurfaceID is allocated in VaapiWrapper.
//        |
// +----->|
// |      v
// | VASurfaceID is put onto VaapiVideoDecodeAccelerator::available_va_surfaces_
// |      |      list.
// |      v
// | VASurfaceID is taken off of the VVDA:available_va_surfaces_ when
// |      |      VaapiH264Decoder requests more output surfaces, is wrapped into
// |      |      a VASurface and passed to VaapiH264Decoder.
// |      v
// | VASurface is put onto VaapiH264Decoder::available_va_surfaces_, keeping
// |      |      the only reference to it until it's needed for decoding.
// |      v
// | VaapiH264Decoder starts decoding a new frame. It takes a VASurface off of
// |      |      VHD::available_va_surfaces_ and assigns it to a DecodeSurface,
// |      |      which now keeps the only reference.
// |      v
// | DecodeSurface is used for decoding, putting data into associated VASurface.
// |      |
// |      |--------------------------------------------------+
// |      |                                                  |
// |      v                                                  v
// | DecodeSurface is to be output.              VaapiH264Decoder uses the
// | VaapiH264Decoder passes the associated      DecodeSurface and associated
// | VASurface to VaapiVideoDecodeAccelerator,   VASurface as reference for
// | which stores it (taking a ref) on           decoding more frames.
// | pending_output_cbs_ queue until an output               |
// | TFPPicture becomes available.                           v
// |                 |                           Once the DecodeSurface is not
// |                 |                           needed as reference anymore,
// |                 v                           it is released, releasing the
// | A TFPPicture becomes available after        associated VASurface reference.
// | the client of VVDA returns                              |
// | a PictureBuffer associated with it. VVDA                |
// | puts the contents of the VASurface into                 |
// | it and releases the reference to VASurface.             |
// |                 |                                       |
// |                 '---------------------------------------'
// |                                     |
// |                                     v
// | Neither VVDA nor VHD hold a reference to VASurface. VASurface is released,
// | ReleaseCB gets called in its destructor, which puts the associated
// | VASurfaceID back onto VVDA::available_va_surfaces_.
// |                                     |
// '-------------------------------------|
//                                       |
//                                       v
//                       VaapiWrapper frees VASurfaceID.
//
class CONTENT_EXPORT VASurface : public base::RefCountedThreadSafe<VASurface> {
 public:
  // Provided by user, will be called when all references to the surface
  // are released.
  typedef base::Callback<void(VASurfaceID)> ReleaseCB;

  VASurface(VASurfaceID va_surface_id, const ReleaseCB& release_cb);

  VASurfaceID id() {
    return va_surface_id_;
  }

 private:
  friend class base::RefCountedThreadSafe<VASurface>;
  ~VASurface();

  const VASurfaceID va_surface_id_;
  ReleaseCB release_cb_;

  DISALLOW_COPY_AND_ASSIGN(VASurface);
};

}  // namespace content

#endif  // CONTENT_COMMON_GPU_MEDIA_VA_SURFACE_H_
