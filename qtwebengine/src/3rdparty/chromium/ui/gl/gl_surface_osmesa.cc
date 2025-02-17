// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "third_party/mesa/src/include/GL/osmesa.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface_osmesa.h"
#include "ui/gl/scoped_make_current.h"

namespace gfx {

GLSurfaceOSMesa::GLSurfaceOSMesa(unsigned format, const gfx::Size& size)
    : format_(format),
      size_(size) {
  // Implementations of OSMesa surface do not support having a 0 size. In such
  // cases use a (1, 1) surface.
  if (size_.GetArea() == 0)
    size_.SetSize(1, 1);
}

bool GLSurfaceOSMesa::Initialize() {
  return Resize(size_);
}

void GLSurfaceOSMesa::Destroy() {
  buffer_.reset();
}

bool GLSurfaceOSMesa::Resize(const gfx::Size& new_size) {
  scoped_ptr<ui::ScopedMakeCurrent> scoped_make_current;
  GLContext* current_context = GLContext::GetCurrent();
  bool was_current =
      current_context && current_context->IsCurrent(this);
  if (was_current) {
    scoped_make_current.reset(
        new ui::ScopedMakeCurrent(current_context, this));
    current_context->ReleaseCurrent(this);
  }

  // Preserve the old buffer.
  scoped_ptr<int32[]> old_buffer(buffer_.release());

  // Allocate a new one.
  buffer_.reset(new int32[new_size.GetArea()]);
  memset(buffer_.get(), 0, new_size.GetArea() * sizeof(buffer_[0]));

  // Copy the old back buffer into the new buffer.
  if (old_buffer.get()) {
    int copy_width = std::min(size_.width(), new_size.width());
    int copy_height = std::min(size_.height(), new_size.height());
    for (int y = 0; y < copy_height; ++y) {
      for (int x = 0; x < copy_width; ++x) {
        buffer_[y * new_size.width() + x] = old_buffer[y * size_.width() + x];
      }
    }
  }

  size_ = new_size;

  return true;
}

bool GLSurfaceOSMesa::IsOffscreen() {
  return true;
}

bool GLSurfaceOSMesa::SwapBuffers() {
  NOTREACHED() << "Should not call SwapBuffers on an GLSurfaceOSMesa.";
  return false;
}

gfx::Size GLSurfaceOSMesa::GetSize() {
  return size_;
}

void* GLSurfaceOSMesa::GetHandle() {
  return buffer_.get();
}

unsigned GLSurfaceOSMesa::GetFormat() {
  return format_;
}

GLSurfaceOSMesa::~GLSurfaceOSMesa() {
  Destroy();
}

bool GLSurfaceOSMesaHeadless::IsOffscreen() { return false; }

bool GLSurfaceOSMesaHeadless::SwapBuffers() { return true; }

GLSurfaceOSMesaHeadless::GLSurfaceOSMesaHeadless()
    : GLSurfaceOSMesa(OSMESA_BGRA, gfx::Size(1, 1)) {}

GLSurfaceOSMesaHeadless::~GLSurfaceOSMesaHeadless() { Destroy(); }

}  // namespace gfx
