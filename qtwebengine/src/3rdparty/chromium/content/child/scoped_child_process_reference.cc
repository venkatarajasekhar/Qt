// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/scoped_child_process_reference.h"

#include "base/bind.h"
#include "base/time/time.h"
#include "content/child/child_process.h"

namespace content {

ScopedChildProcessReference::ScopedChildProcessReference()
    : has_reference_(true) {
  ChildProcess::current()->AddRefProcess();
}

ScopedChildProcessReference::~ScopedChildProcessReference() {
  if (has_reference_)
    ChildProcess::current()->ReleaseProcess();
}

void ScopedChildProcessReference::ReleaseWithDelay(
    const base::TimeDelta& delay) {
  DCHECK(has_reference_);
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ChildProcess::ReleaseProcess,
                 base::Unretained(ChildProcess::current())),
      delay);
  has_reference_ = false;
}

}  // namespace content
