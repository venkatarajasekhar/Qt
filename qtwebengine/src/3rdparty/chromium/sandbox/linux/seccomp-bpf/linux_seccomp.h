// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SECCOMP_BPF_LINUX_SECCOMP_H__
#define SANDBOX_LINUX_SECCOMP_BPF_LINUX_SECCOMP_H__

// The Seccomp2 kernel ABI is not part of older versions of glibc.
// As we can't break compilation with these versions of the library,
// we explicitly define all missing symbols.
// If we ever decide that we can now rely on system headers, the following
// include files should be enabled:
// #include <linux/audit.h>
// #include <linux/seccomp.h>

#include <asm/unistd.h>
#include <linux/filter.h>

#include <sys/cdefs.h>
// Old Bionic versions do not have sys/user.h.  The if can be removed once we no
// longer need to support these old Bionic versions.
// All x86_64 builds use a new enough bionic to have sys/user.h.
#if !defined(__BIONIC__) || defined(__x86_64__)
#include <sys/user.h>
#endif

// For audit.h
#ifndef EM_ARM
#define EM_ARM    40
#endif
#ifndef EM_386
#define EM_386    3
#endif
#ifndef EM_X86_64
#define EM_X86_64 62
#endif

#ifndef __AUDIT_ARCH_64BIT
#define __AUDIT_ARCH_64BIT 0x80000000
#endif
#ifndef __AUDIT_ARCH_LE
#define __AUDIT_ARCH_LE    0x40000000
#endif
#ifndef AUDIT_ARCH_ARM
#define AUDIT_ARCH_ARM    (EM_ARM|__AUDIT_ARCH_LE)
#endif
#ifndef AUDIT_ARCH_I386
#define AUDIT_ARCH_I386   (EM_386|__AUDIT_ARCH_LE)
#endif
#ifndef AUDIT_ARCH_X86_64
#define AUDIT_ARCH_X86_64 (EM_X86_64|__AUDIT_ARCH_64BIT|__AUDIT_ARCH_LE)
#endif

// For prctl.h
#ifndef PR_SET_SECCOMP
#define PR_SET_SECCOMP               22
#define PR_GET_SECCOMP               21
#endif
#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS          38
#define PR_GET_NO_NEW_PRIVS          39
#endif
#ifndef IPC_64
#define IPC_64                   0x0100
#endif

#ifndef BPF_MOD
#define BPF_MOD                    0x90
#endif
#ifndef BPF_XOR
#define BPF_XOR                    0xA0
#endif

// In order to build will older tool chains, we currently have to avoid
// including <linux/seccomp.h>. Until that can be fixed (if ever). Rely on
// our own definitions of the seccomp kernel ABI.
#ifndef SECCOMP_MODE_FILTER
#define SECCOMP_MODE_DISABLED         0
#define SECCOMP_MODE_STRICT           1
#define SECCOMP_MODE_FILTER           2  // User user-supplied filter
#endif

#ifndef SECCOMP_RET_KILL
// Return values supported for BPF filter programs. Please note that the
// "illegal" SECCOMP_RET_INVALID is not supported by the kernel, should only
// ever be used internally, and would result in the kernel killing our process.
#define SECCOMP_RET_KILL    0x00000000U  // Kill the task immediately
#define SECCOMP_RET_INVALID 0x00010000U  // Illegal return value
#define SECCOMP_RET_TRAP    0x00030000U  // Disallow and force a SIGSYS
#define SECCOMP_RET_ERRNO   0x00050000U  // Returns an errno
#define SECCOMP_RET_TRACE   0x7ff00000U  // Pass to a tracer or disallow
#define SECCOMP_RET_ALLOW   0x7fff0000U  // Allow
#define SECCOMP_RET_ACTION  0xffff0000U  // Masks for the return value
#define SECCOMP_RET_DATA    0x0000ffffU  //   sections
#else
#define SECCOMP_RET_INVALID 0x00010000U  // Illegal return value
#endif

#ifndef SYS_SECCOMP
#define SYS_SECCOMP                   1
#endif

// Impose some reasonable maximum BPF program size. Realistically, the
// kernel probably has much lower limits. But by limiting to less than
// 30 bits, we can ease requirements on some of our data types.
#define SECCOMP_MAX_PROGRAM_SIZE (1<<30)

#if defined(__i386__)
#define MIN_SYSCALL         0u
#define MAX_PUBLIC_SYSCALL  1024u
#define MAX_SYSCALL         MAX_PUBLIC_SYSCALL
#define SECCOMP_ARCH        AUDIT_ARCH_I386

#define SECCOMP_REG(_ctx, _reg) ((_ctx)->uc_mcontext.gregs[(_reg)])
#define SECCOMP_RESULT(_ctx)    SECCOMP_REG(_ctx, REG_EAX)
#define SECCOMP_SYSCALL(_ctx)   SECCOMP_REG(_ctx, REG_EAX)
#define SECCOMP_IP(_ctx)        SECCOMP_REG(_ctx, REG_EIP)
#define SECCOMP_PARM1(_ctx)     SECCOMP_REG(_ctx, REG_EBX)
#define SECCOMP_PARM2(_ctx)     SECCOMP_REG(_ctx, REG_ECX)
#define SECCOMP_PARM3(_ctx)     SECCOMP_REG(_ctx, REG_EDX)
#define SECCOMP_PARM4(_ctx)     SECCOMP_REG(_ctx, REG_ESI)
#define SECCOMP_PARM5(_ctx)     SECCOMP_REG(_ctx, REG_EDI)
#define SECCOMP_PARM6(_ctx)     SECCOMP_REG(_ctx, REG_EBP)
#define SECCOMP_NR_IDX          (offsetof(struct arch_seccomp_data, nr))
#define SECCOMP_ARCH_IDX        (offsetof(struct arch_seccomp_data, arch))
#define SECCOMP_IP_MSB_IDX      (offsetof(struct arch_seccomp_data,           \
                                          instruction_pointer) + 4)
#define SECCOMP_IP_LSB_IDX      (offsetof(struct arch_seccomp_data,           \
                                          instruction_pointer) + 0)
#define SECCOMP_ARG_MSB_IDX(nr) (offsetof(struct arch_seccomp_data, args) +   \
                                 8*(nr) + 4)
#define SECCOMP_ARG_LSB_IDX(nr) (offsetof(struct arch_seccomp_data, args) +   \
                                 8*(nr) + 0)


#if defined(__BIONIC__)
// Old Bionic versions don't have sys/user.h, so we just define regs_struct
// directly.  This can be removed once we no longer need to support these old
// Bionic versions.
struct regs_struct {
  long int ebx;
  long int ecx;
  long int edx;
  long int esi;
  long int edi;
  long int ebp;
  long int eax;
  long int xds;
  long int xes;
  long int xfs;
  long int xgs;
  long int orig_eax;
  long int eip;
  long int xcs;
  long int eflags;
  long int esp;
  long int xss;
};
#else
typedef user_regs_struct regs_struct;
#endif

#define SECCOMP_PT_RESULT(_regs)  (_regs).eax
#define SECCOMP_PT_SYSCALL(_regs) (_regs).orig_eax
#define SECCOMP_PT_IP(_regs)      (_regs).eip
#define SECCOMP_PT_PARM1(_regs)   (_regs).ebx
#define SECCOMP_PT_PARM2(_regs)   (_regs).ecx
#define SECCOMP_PT_PARM3(_regs)   (_regs).edx
#define SECCOMP_PT_PARM4(_regs)   (_regs).esi
#define SECCOMP_PT_PARM5(_regs)   (_regs).edi
#define SECCOMP_PT_PARM6(_regs)   (_regs).ebp

#elif defined(__x86_64__)
#define MIN_SYSCALL         0u
#define MAX_PUBLIC_SYSCALL  1024u
#define MAX_SYSCALL         MAX_PUBLIC_SYSCALL
#define SECCOMP_ARCH        AUDIT_ARCH_X86_64

#define SECCOMP_REG(_ctx, _reg) ((_ctx)->uc_mcontext.gregs[(_reg)])
#define SECCOMP_RESULT(_ctx)    SECCOMP_REG(_ctx, REG_RAX)
#define SECCOMP_SYSCALL(_ctx)   SECCOMP_REG(_ctx, REG_RAX)
#define SECCOMP_IP(_ctx)        SECCOMP_REG(_ctx, REG_RIP)
#define SECCOMP_PARM1(_ctx)     SECCOMP_REG(_ctx, REG_RDI)
#define SECCOMP_PARM2(_ctx)     SECCOMP_REG(_ctx, REG_RSI)
#define SECCOMP_PARM3(_ctx)     SECCOMP_REG(_ctx, REG_RDX)
#define SECCOMP_PARM4(_ctx)     SECCOMP_REG(_ctx, REG_R10)
#define SECCOMP_PARM5(_ctx)     SECCOMP_REG(_ctx, REG_R8)
#define SECCOMP_PARM6(_ctx)     SECCOMP_REG(_ctx, REG_R9)
#define SECCOMP_NR_IDX          (offsetof(struct arch_seccomp_data, nr))
#define SECCOMP_ARCH_IDX        (offsetof(struct arch_seccomp_data, arch))
#define SECCOMP_IP_MSB_IDX      (offsetof(struct arch_seccomp_data,           \
                                          instruction_pointer) + 4)
#define SECCOMP_IP_LSB_IDX      (offsetof(struct arch_seccomp_data,           \
                                          instruction_pointer) + 0)
#define SECCOMP_ARG_MSB_IDX(nr) (offsetof(struct arch_seccomp_data, args) +   \
                                 8*(nr) + 4)
#define SECCOMP_ARG_LSB_IDX(nr) (offsetof(struct arch_seccomp_data, args) +   \
                                 8*(nr) + 0)

typedef user_regs_struct regs_struct;
#define SECCOMP_PT_RESULT(_regs)  (_regs).rax
#define SECCOMP_PT_SYSCALL(_regs) (_regs).orig_rax
#define SECCOMP_PT_IP(_regs)      (_regs).rip
#define SECCOMP_PT_PARM1(_regs)   (_regs).rdi
#define SECCOMP_PT_PARM2(_regs)   (_regs).rsi
#define SECCOMP_PT_PARM3(_regs)   (_regs).rdx
#define SECCOMP_PT_PARM4(_regs)   (_regs).r10
#define SECCOMP_PT_PARM5(_regs)   (_regs).r8
#define SECCOMP_PT_PARM6(_regs)   (_regs).r9

#elif defined(__arm__) && (defined(__thumb__) || defined(__ARM_EABI__))
// ARM EABI includes "ARM private" system calls starting at |__ARM_NR_BASE|,
// and a "ghost syscall private to the kernel", cmpxchg,
// at |__ARM_NR_BASE+0x00fff0|.
// See </arch/arm/include/asm/unistd.h> in the Linux kernel.
#define MIN_SYSCALL         ((unsigned int)__NR_SYSCALL_BASE)
#define MAX_PUBLIC_SYSCALL  (MIN_SYSCALL + 1024u)
#define MIN_PRIVATE_SYSCALL ((unsigned int)__ARM_NR_BASE)
#define MAX_PRIVATE_SYSCALL (MIN_PRIVATE_SYSCALL + 16u)
#define MIN_GHOST_SYSCALL   ((unsigned int)__ARM_NR_BASE + 0xfff0u)
#define MAX_SYSCALL         (MIN_GHOST_SYSCALL + 4u)

#define SECCOMP_ARCH AUDIT_ARCH_ARM

// ARM sigcontext_t is different from i386/x86_64.
// See </arch/arm/include/asm/sigcontext.h> in the Linux kernel.
#define SECCOMP_REG(_ctx, _reg) ((_ctx)->uc_mcontext.arm_##_reg)
// ARM EABI syscall convention.
#define SECCOMP_RESULT(_ctx)    SECCOMP_REG(_ctx, r0)
#define SECCOMP_SYSCALL(_ctx)   SECCOMP_REG(_ctx, r7)
#define SECCOMP_IP(_ctx)        SECCOMP_REG(_ctx, pc)
#define SECCOMP_PARM1(_ctx)     SECCOMP_REG(_ctx, r0)
#define SECCOMP_PARM2(_ctx)     SECCOMP_REG(_ctx, r1)
#define SECCOMP_PARM3(_ctx)     SECCOMP_REG(_ctx, r2)
#define SECCOMP_PARM4(_ctx)     SECCOMP_REG(_ctx, r3)
#define SECCOMP_PARM5(_ctx)     SECCOMP_REG(_ctx, r4)
#define SECCOMP_PARM6(_ctx)     SECCOMP_REG(_ctx, r5)
#define SECCOMP_NR_IDX          (offsetof(struct arch_seccomp_data, nr))
#define SECCOMP_ARCH_IDX        (offsetof(struct arch_seccomp_data, arch))
#define SECCOMP_IP_MSB_IDX      (offsetof(struct arch_seccomp_data,           \
                                          instruction_pointer) + 4)
#define SECCOMP_IP_LSB_IDX      (offsetof(struct arch_seccomp_data,           \
                                          instruction_pointer) + 0)
#define SECCOMP_ARG_MSB_IDX(nr) (offsetof(struct arch_seccomp_data, args) +   \
                                 8*(nr) + 4)
#define SECCOMP_ARG_LSB_IDX(nr) (offsetof(struct arch_seccomp_data, args) +   \
                                 8*(nr) + 0)

#if defined(__BIONIC__)
// Old Bionic versions don't have sys/user.h, so we just define regs_struct
// directly.  This can be removed once we no longer need to support these old
// Bionic versions.
struct regs_struct {
  unsigned long uregs[18];
};
#else
typedef user_regs regs_struct;
#endif

#define REG_cpsr    uregs[16]
#define REG_pc      uregs[15]
#define REG_lr      uregs[14]
#define REG_sp      uregs[13]
#define REG_ip      uregs[12]
#define REG_fp      uregs[11]
#define REG_r10     uregs[10]
#define REG_r9      uregs[9]
#define REG_r8      uregs[8]
#define REG_r7      uregs[7]
#define REG_r6      uregs[6]
#define REG_r5      uregs[5]
#define REG_r4      uregs[4]
#define REG_r3      uregs[3]
#define REG_r2      uregs[2]
#define REG_r1      uregs[1]
#define REG_r0      uregs[0]
#define REG_ORIG_r0 uregs[17]

#define SECCOMP_PT_RESULT(_regs)  (_regs).REG_r0
#define SECCOMP_PT_SYSCALL(_regs) (_regs).REG_r7
#define SECCOMP_PT_IP(_regs)      (_regs).REG_pc
#define SECCOMP_PT_PARM1(_regs)   (_regs).REG_r0
#define SECCOMP_PT_PARM2(_regs)   (_regs).REG_r1
#define SECCOMP_PT_PARM3(_regs)   (_regs).REG_r2
#define SECCOMP_PT_PARM4(_regs)   (_regs).REG_r3
#define SECCOMP_PT_PARM5(_regs)   (_regs).REG_r4
#define SECCOMP_PT_PARM6(_regs)   (_regs).REG_r5

#else
#error Unsupported target platform

#endif

#endif  // SANDBOX_LINUX_SECCOMP_BPF_LINUX_SECCOMP_H__
