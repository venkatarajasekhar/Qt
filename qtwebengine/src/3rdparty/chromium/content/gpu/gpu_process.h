// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_GPU_GPU_PROCESS_H_
#define CONTENT_GPU_GPU_PROCESS_H_

#include "content/child/child_process.h"

namespace content {

class GpuProcess : public ChildProcess {
 public:
  GpuProcess();
  virtual ~GpuProcess();

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuProcess);
};

}

#endif  // CONTENT_GPU_GPU_PROCESS_H_
