// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SECCOMP_BPF_HELPERS_SIGSYS_HANDLERS_H_
#define SANDBOX_LINUX_SECCOMP_BPF_HELPERS_SIGSYS_HANDLERS_H_

#include "base/basictypes.h"
#include "build/build_config.h"
#include "sandbox/sandbox_export.h"

// The handlers are suitable for use in Trap() error codes. They are
// guaranteed to be async-signal safe.
// See sandbox/linux/seccomp-bpf/trap.h to see how they work.

namespace sandbox {

struct arch_seccomp_data;

// This handler will crash the currently running process. The crashing address
// will be the number of the current system call, extracted from |args|.
// This handler will also print to stderr the number of the crashing syscall.
SANDBOX_EXPORT intptr_t
    CrashSIGSYS_Handler(const struct arch_seccomp_data& args, void* aux);

// The following three handlers are suitable to report failures with the
// clone(), prctl() and ioctl() system calls respectively.

// The crashing address will be (clone_flags & 0xFFFFFF), where clone_flags is
// the clone(2) argument, extracted from |args|.
SANDBOX_EXPORT intptr_t
    SIGSYSCloneFailure(const struct arch_seccomp_data& args, void* aux);
// The crashing address will be (option & 0xFFF), where option is the prctl(2)
// argument.
SANDBOX_EXPORT intptr_t
    SIGSYSPrctlFailure(const struct arch_seccomp_data& args, void* aux);
// The crashing address will be request & 0xFFFF, where request is the ioctl(2)
// argument.
SANDBOX_EXPORT intptr_t
    SIGSYSIoctlFailure(const struct arch_seccomp_data& args, void* aux);
// The crashing address will be (pid & 0xFFF), where pid is the first
// argument (and can be a tid).
SANDBOX_EXPORT intptr_t
    SIGSYSKillFailure(const struct arch_seccomp_data& args, void* aux);
// The crashing address will be (op & 0xFFF), where op is the second
// argument.
SANDBOX_EXPORT intptr_t
    SIGSYSFutexFailure(const struct arch_seccomp_data& args, void* aux);

// Following four functions return substrings of error messages used
// in the above four functions. They are useful in death tests.
SANDBOX_EXPORT const char* GetErrorMessageContentForTests();
SANDBOX_EXPORT const char* GetCloneErrorMessageContentForTests();
SANDBOX_EXPORT const char* GetPrctlErrorMessageContentForTests();
SANDBOX_EXPORT const char* GetIoctlErrorMessageContentForTests();
SANDBOX_EXPORT const char* GetKillErrorMessageContentForTests();
SANDBOX_EXPORT const char* GetFutexErrorMessageContentForTests();

}  // namespace sandbox.

#endif  // SANDBOX_LINUX_SECCOMP_BPF_HELPERS_SIGSYS_HANDLERS_H_
