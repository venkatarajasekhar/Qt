// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SPEECH_CHUNKED_BYTE_BUFFER_H_
#define CONTENT_BROWSER_SPEECH_CHUNKED_BYTE_BUFFER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "content/common/content_export.h"

namespace content {

// Models a chunk-oriented byte buffer. The term chunk is herein defined as an
// arbitrary sequence of bytes that is preceeded by N header bytes, indicating
// its size. Data may be appended to the buffer with no particular respect of
// chunks boundaries. However, chunks can be extracted (FIFO) only when their
// content (according to their header) is fully available in the buffer.
// The current implementation support only 4 byte Big Endian headers.
// Empty chunks (i.e. the sequence 00 00 00 00) are NOT allowed.
//
// E.g. 00 00 00 04 xx xx xx xx 00 00 00 02 yy yy 00 00 00 04 zz zz zz zz
//      [----- CHUNK 1 -------] [--- CHUNK 2 ---] [------ CHUNK 3 ------]
class CONTENT_EXPORT ChunkedByteBuffer {
 public:
  ChunkedByteBuffer();
  ~ChunkedByteBuffer();

  // Appends |length| bytes starting from |start| to the buffer.
  void Append(const uint8* start, size_t length);

  // Appends bytes contained in the |string| to the buffer.
  void Append(const std::string& string);

  // Checks whether one or more complete chunks are available in the buffer.
  bool HasChunks() const;

  // If enough data is available, reads and removes the first complete chunk
  // from the buffer. Returns a NULL pointer if no complete chunk is available.
  scoped_ptr< std::vector<uint8> > PopChunk();

  // Clears all the content of the buffer.
  void Clear();

  // Returns the number of raw bytes (including headers) present.
  size_t GetTotalLength() const { return total_bytes_stored_; }

 private:
  struct Chunk {
    Chunk();
    ~Chunk();

    std::vector<uint8> header;
    scoped_ptr< std::vector<uint8> > content;
    size_t ExpectedContentLength() const;

   private:
    DISALLOW_COPY_AND_ASSIGN(Chunk);
  };

  ScopedVector<Chunk> chunks_;
  scoped_ptr<Chunk> partial_chunk_;
  size_t total_bytes_stored_;

  DISALLOW_COPY_AND_ASSIGN(ChunkedByteBuffer);
};


}  // namespace content

#endif  // CONTENT_BROWSER_SPEECH_CHUNKED_BYTE_BUFFER_H_
