// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_LINUX_ANDROID_SANDBOX_BPF_BASE_POLICY_ANDROID_H_
#define CONTENT_COMMON_SANDBOX_LINUX_ANDROID_SANDBOX_BPF_BASE_POLICY_ANDROID_H_

#include "content/common/sandbox_linux/sandbox_bpf_base_policy_linux.h"
#include "sandbox/linux/seccomp-bpf/errorcode.h"

namespace content {

// This class builds on top of the generic Linux baseline policy to reduce
// Linux kernel attack surface. It augments the list of allowed syscalls to
// allow ones required by the Android runtime.
class SandboxBPFBasePolicyAndroid : public SandboxBPFBasePolicy {
 public:
  SandboxBPFBasePolicyAndroid();
  virtual ~SandboxBPFBasePolicyAndroid();

  // sandbox::SandboxBPFPolicy:
  virtual sandbox::ErrorCode EvaluateSyscall(
      sandbox::SandboxBPF* sandbox_compiler,
      int system_call_number) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(SandboxBPFBasePolicyAndroid);
};

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_LINUX_ANDROID_SANDBOX_BPF_BASE_POLICY_ANDROID_H_
