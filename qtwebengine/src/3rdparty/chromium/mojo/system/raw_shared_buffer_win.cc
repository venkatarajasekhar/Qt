// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/system/raw_shared_buffer.h"

#include <windows.h>

#include <limits>

#include "base/logging.h"
#include "base/sys_info.h"
#include "mojo/embedder/platform_handle.h"
#include "mojo/embedder/scoped_platform_handle.h"

namespace mojo {
namespace system {

// RawSharedBuffer -------------------------------------------------------------

bool RawSharedBuffer::Init() {
  DCHECK(!handle_.is_valid());

  // TODO(vtl): Currently, we only support mapping up to 2^32-1 bytes.
  if (static_cast<uint64_t>(num_bytes_) >
          static_cast<uint64_t>(std::numeric_limits<DWORD>::max())) {
    return false;
  }

  // IMPORTANT NOTE: Unnamed objects are NOT SECURABLE. Thus if we ever want to
  // share read-only to other processes, we'll have to name our file mapping
  // object.
  // TODO(vtl): Unlike |base::SharedMemory|, we don't round up the size (to a
  // multiple of 64 KB). This may cause problems with NaCl. Cross this bridge
  // when we get there. crbug.com/210609
  handle_.reset(embedder::PlatformHandle(CreateFileMapping(
      INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
      static_cast<DWORD>(num_bytes_), NULL)));
  if (!handle_.is_valid()) {
    PLOG(ERROR) << "CreateFileMapping";
    return false;
  }

  return true;
}

bool RawSharedBuffer::InitFromPlatformHandle(
    embedder::ScopedPlatformHandle platform_handle) {
  DCHECK(!handle_.is_valid());

  // TODO(vtl): Implement.
  NOTIMPLEMENTED();
  return false;
}

scoped_ptr<RawSharedBufferMapping> RawSharedBuffer::MapImpl(size_t offset,
                                                            size_t length) {
  size_t offset_rounding = offset % base::SysInfo::VMAllocationGranularity();
  size_t real_offset = offset - offset_rounding;
  size_t real_length = length + offset_rounding;

  // This should hold (since we checked |num_bytes| versus the maximum value of
  // |off_t| on creation, but it never hurts to be paranoid.
  DCHECK_LE(static_cast<uint64_t>(real_offset),
            static_cast<uint64_t>(std::numeric_limits<DWORD>::max()));

  void* real_base = MapViewOfFile(
      handle_.get().handle, FILE_MAP_READ | FILE_MAP_WRITE, 0,
      static_cast<DWORD>(real_offset), real_length);
  if (!real_base) {
    PLOG(ERROR) << "MapViewOfFile";
    return scoped_ptr<RawSharedBufferMapping>();
  }

  void* base = static_cast<char*>(real_base) + offset_rounding;
  return make_scoped_ptr(
      new RawSharedBufferMapping(base, length, real_base, real_length));
}

// RawSharedBufferMapping ------------------------------------------------------

void RawSharedBufferMapping::Unmap() {
  BOOL result = UnmapViewOfFile(real_base_);
  PLOG_IF(ERROR, !result) << "UnmapViewOfFile";
}

}  // namespace system
}  // namespace mojo
