// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_STUB_H_
#define UI_GL_GL_SURFACE_STUB_H_

#include "ui/gl/gl_surface.h"

namespace gfx {

// A GLSurface that does nothing for unit tests.
class GL_EXPORT GLSurfaceStub : public GLSurface {
 public:
  void SetSize(const gfx::Size& size) { size_ = size; }

  // Implement GLSurface.
  virtual void Destroy() OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void* GetHandle() OVERRIDE;

 protected:
  virtual ~GLSurfaceStub();

 private:
  gfx::Size size_;
};

}  // namespace gfx

#endif  // UI_GL_GL_SURFACE_STUB_H_
