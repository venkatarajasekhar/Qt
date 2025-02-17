// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/async_pixel_transfer_manager.h"

#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "gpu/command_buffer/service/async_pixel_transfer_manager_idle.h"
#include "gpu/command_buffer/service/async_pixel_transfer_manager_share_group.h"
#include "gpu/command_buffer/service/async_pixel_transfer_manager_stub.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "ui/gl/gl_implementation.h"

namespace gpu {

AsyncPixelTransferManager* AsyncPixelTransferManager::Create(
    gfx::GLContext* context) {
  TRACE_EVENT0("gpu", "AsyncPixelTransferManager::Create");
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableShareGroupAsyncTextureUpload)) {
    DCHECK(context);
    return static_cast<AsyncPixelTransferManager*> (
        new AsyncPixelTransferManagerShareGroup(context));
  }

  switch (gfx::GetGLImplementation()) {
    case gfx::kGLImplementationOSMesaGL:
    case gfx::kGLImplementationDesktopGL:
    case gfx::kGLImplementationEGLGLES2:
      return new AsyncPixelTransferManagerIdle;
    case gfx::kGLImplementationMockGL:
      return new AsyncPixelTransferManagerStub;
    default:
      NOTREACHED();
      return NULL;
  }
}

}  // namespace gpu
