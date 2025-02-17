// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_OSMESA_H_
#define UI_GL_GL_SURFACE_OSMESA_H_

#include "base/memory/scoped_ptr.h"
#include "ui/gfx/size.h"
#include "ui/gl/gl_surface.h"

namespace gfx {

// A surface that the Mesa software renderer draws to. This is actually just a
// buffer in system memory. GetHandle returns a pointer to the buffer. These
// surfaces can be resized and resizing preserves the contents.
class GL_EXPORT GLSurfaceOSMesa : public GLSurface {
 public:
  GLSurfaceOSMesa(unsigned format, const gfx::Size& size);

  // Implement GLSurface.
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool Resize(const gfx::Size& new_size) OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void* GetHandle() OVERRIDE;
  virtual unsigned GetFormat() OVERRIDE;

 protected:
  virtual ~GLSurfaceOSMesa();

 private:
  unsigned format_;
  gfx::Size size_;
  scoped_ptr<int32[]> buffer_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesa);
};

// A thin subclass of |GLSurfaceOSMesa| that can be used in place
// of a native hardware-provided surface when a native surface
// provider is not available.
class GLSurfaceOSMesaHeadless : public GLSurfaceOSMesa {
 public:
  explicit GLSurfaceOSMesaHeadless();

  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;

 protected:
  virtual ~GLSurfaceOSMesaHeadless();

 private:
  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesaHeadless);
};

}  // namespace gfx

#endif  // UI_GL_GL_SURFACE_OSMESA_H_
