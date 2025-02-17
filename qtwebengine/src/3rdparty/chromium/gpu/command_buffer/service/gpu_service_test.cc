// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_service_test.h"

#include "gpu/command_buffer/service/test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_context_stub_with_extensions.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_mock.h"
#include "ui/gl/gl_surface.h"

namespace gpu {
namespace gles2 {

GpuServiceTest::GpuServiceTest() : ran_setup_(false), ran_teardown_(false) {
}

GpuServiceTest::~GpuServiceTest() {
  DCHECK(ran_teardown_);
}

void GpuServiceTest::SetUp() {
  testing::Test::SetUp();

  gfx::SetGLGetProcAddressProc(gfx::MockGLInterface::GetGLProcAddress);
  gfx::GLSurface::InitializeOneOffWithMockBindingsForTests();
  gl_.reset(new ::testing::StrictMock< ::gfx::MockGLInterface>());
  ::gfx::MockGLInterface::SetGLInterface(gl_.get());

  context_ = new gfx::GLContextStubWithExtensions;
  context_->AddExtensionsString(NULL);
  context_->SetGLVersionString("3.0");
  gfx::GLSurface::InitializeDynamicMockBindingsForTests(context_);
  ran_setup_ = true;
}

void GpuServiceTest::TearDown() {
  DCHECK(ran_setup_);
  ::gfx::MockGLInterface::SetGLInterface(NULL);
  gl_.reset();
  gfx::ClearGLBindings();
  ran_teardown_ = true;

  testing::Test::TearDown();
}

}  // namespace gles2
}  // namespace gpu
