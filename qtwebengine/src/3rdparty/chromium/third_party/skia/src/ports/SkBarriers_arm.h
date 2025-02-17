/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkBarriers_arm_DEFINED
#define SkBarriers_arm_DEFINED

static inline void sk_compiler_barrier() { asm volatile("" : : : "memory"); }

template <typename T>
T sk_acquire_load(T* ptr) {
    T val = *ptr;
    __sync_synchronize();  // Issue a full barrier, which is an overkill acquire barrier.
    return val;
}

template <typename T>
void sk_release_store(T* ptr, T val) {
    __sync_synchronize();  // Issue a full barrier, which is an overkill release barrier.
    *ptr = val;
}

#endif//SkBarriers_x86_DEFINED
