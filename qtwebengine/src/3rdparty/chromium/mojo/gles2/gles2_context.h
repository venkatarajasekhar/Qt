// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_GLES2_GLES2_CONTEXT_H_
#define MOJO_GLES2_GLES2_CONTEXT_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "mojo/gles2/command_buffer_client_impl.h"
#include "mojo/public/c/gles2/gles2.h"

struct MojoGLES2ContextPrivate {};

namespace gpu {
class TransferBuffer;
namespace gles2 {
class GLES2CmdHelper;
class GLES2Implementation;
}
}

namespace mojo {
namespace gles2 {

class GLES2Context : public CommandBufferDelegate,
                     public MojoGLES2ContextPrivate {
 public:
  explicit GLES2Context(const MojoAsyncWaiter* async_waiter,
                        ScopedMessagePipeHandle command_buffer_handle,
                        MojoGLES2ContextLost lost_callback,
                        MojoGLES2DrawAnimationFrame animation_callback,
                        void* closure);
  virtual ~GLES2Context();
  bool Initialize();

  gpu::gles2::GLES2Interface* interface() const {
    return implementation_.get();
  }
  gpu::ContextSupport* context_support() const { return implementation_.get(); }
  void RequestAnimationFrames();
  void CancelAnimationFrames();

 private:
  virtual void ContextLost() OVERRIDE;
  virtual void DrawAnimationFrame() OVERRIDE;

  CommandBufferClientImpl command_buffer_;
  scoped_ptr<gpu::gles2::GLES2CmdHelper> gles2_helper_;
  scoped_ptr<gpu::TransferBuffer> transfer_buffer_;
  scoped_ptr<gpu::gles2::GLES2Implementation> implementation_;
  MojoGLES2ContextLost lost_callback_;
  MojoGLES2DrawAnimationFrame animation_callback_;
  void* closure_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(GLES2Context);
};

}  // namespace gles2
}  // namespace mojo

#endif  // MOJO_GLES2_GLES2_CONTEXT_H_
