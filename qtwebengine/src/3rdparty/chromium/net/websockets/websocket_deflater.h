// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_DEFLATER_H_
#define NET_WEBSOCKETS_WEBSOCKET_DEFLATER_H_

#include <deque>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/net_export.h"

extern "C" struct z_stream_s;

namespace net {

class IOBufferWithSize;

class NET_EXPORT_PRIVATE WebSocketDeflater {
 public:
  // Do not reorder or remove entries of this enum. The values of them are used
  // in UMA.
  enum ContextTakeOverMode {
    DO_NOT_TAKE_OVER_CONTEXT,
    TAKE_OVER_CONTEXT,
    NUM_CONTEXT_TAKEOVER_MODE_TYPES,
  };

  explicit WebSocketDeflater(ContextTakeOverMode mode);
  ~WebSocketDeflater();

  // Returns true if there is no error and false otherwise.
  // This function must be called exactly once before calling any of
  // following methods.
  // |window_bits| must be between 8 and 15 (both inclusive).
  bool Initialize(int window_bits);

  // Adds bytes to |stream_|.
  // Returns true if there is no error and false otherwise.
  bool AddBytes(const char* data, size_t size);

  // Flushes the current processing data.
  // Returns true if there is no error and false otherwise.
  bool Finish();

  // Pushes "\x00\x00\xff\xff" to the end of the buffer.
  void PushSyncMark();

  // Returns the current deflated output.
  // If the current output is larger than |size| bytes,
  // returns the first |size| bytes of the current output.
  // The returned bytes will be dropped from the current output and never be
  // returned thereafter.
  scoped_refptr<IOBufferWithSize> GetOutput(size_t size);

  // Returns the size of the current deflated output.
  size_t CurrentOutputSize() const { return buffer_.size(); }

 private:
  void ResetContext();
  int Deflate(int flush);

  scoped_ptr<z_stream_s> stream_;
  ContextTakeOverMode mode_;
  std::deque<char> buffer_;
  std::vector<char> fixed_buffer_;
  // true if bytes were added after last Finish().
  bool are_bytes_added_;

  DISALLOW_COPY_AND_ASSIGN(WebSocketDeflater);
};

}  // namespace net

#endif  // NET_WEBSOCKETS_WEBSOCKET_DEFLATER_H_
