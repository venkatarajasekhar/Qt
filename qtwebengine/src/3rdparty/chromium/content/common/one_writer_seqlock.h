// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_ONE_WRITER_SEQLOCK_H_
#define CONTENT_COMMON_ONE_WRITER_SEQLOCK_H_

#include "base/atomicops.h"
#include "base/threading/platform_thread.h"
#include "content/common/content_export.h"

namespace content {

// This SeqLock handles only *one* writer and multiple readers. It may be
// suitable for low-contention with relatively infrequent writes, and many
// readers. See:
//   http://en.wikipedia.org/wiki/Seqlock
//   http://www.concurrencykit.org/doc/ck_sequence.html
// This implementation is based on ck_sequence.h from http://concurrencykit.org.
//
// Currently this type of lock is used in two implementations (gamepad and
// device motion, in particular see e.g. shared_memory_seqlock_buffer.h).
// It may make sense to generalize this lock to multiple writers.
//
// You must be very careful not to operate on potentially inconsistent read
// buffers. If the read must be retry'd, the data in the read buffer could
// contain any random garbage. e.g., contained pointers might be
// garbage, or indices could be out of range. Probably the only suitable thing
// to do during the read loop is to make a copy of the data, and operate on it
// only after the read was found to be consistent.
class CONTENT_EXPORT OneWriterSeqLock {
 public:
  OneWriterSeqLock();
  base::subtle::Atomic32 ReadBegin();
  bool ReadRetry(base::subtle::Atomic32 version);
  void WriteBegin();
  void WriteEnd();

 private:
  base::subtle::Atomic32 sequence_;
  DISALLOW_COPY_AND_ASSIGN(OneWriterSeqLock);
};

}  // namespace content

#endif  // CONTENT_COMMON_ONE_WRITER_SEQLOCK_H_
