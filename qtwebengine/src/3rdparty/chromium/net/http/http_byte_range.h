// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_BYTE_RANGE_H_
#define NET_HTTP_HTTP_BYTE_RANGE_H_

#include <string>

#include "base/basictypes.h"
#include "net/base/net_export.h"

namespace net {

// A container class that represents a "range" specified for range request
// specified by RFC 7233 Section 2.1.
// https://tools.ietf.org/html/rfc7233#section-2.1
class NET_EXPORT HttpByteRange {
 public:
  HttpByteRange();

  // Convenience constructors.
  static HttpByteRange Bounded(int64 first_byte_position,
                               int64 last_byte_position);
  static HttpByteRange RightUnbounded(int64 first_byte_position);
  static HttpByteRange Suffix(int64 suffix_length);

  // Since this class is POD, we use constructor, assignment operator
  // and destructor provided by compiler.
  int64 first_byte_position() const { return first_byte_position_; }
  void set_first_byte_position(int64 value) { first_byte_position_ = value; }

  int64 last_byte_position() const { return last_byte_position_; }
  void set_last_byte_position(int64 value) { last_byte_position_ = value; }

  int64 suffix_length() const { return suffix_length_; }
  void set_suffix_length(int64 value) { suffix_length_ = value; }

  // Returns true if this is a suffix byte range.
  bool IsSuffixByteRange() const;
  // Returns true if the first byte position is specified in this request.
  bool HasFirstBytePosition() const;
  // Returns true if the last byte position is specified in this request.
  bool HasLastBytePosition() const;

  // Returns true if this range is valid.
  bool IsValid() const;

  // Gets the header string, e.g. "bytes=0-100", "bytes=100-", "bytes=-100".
  // Assumes range is valid.
  std::string GetHeaderValue() const;

  // A method that when given the size in bytes of a file, adjust the internal
  // |first_byte_position_| and |last_byte_position_| values according to the
  // range specified by this object. If the range specified is invalid with
  // regard to the size or |size| is negative, returns false and there will be
  // no side effect.
  // Returns false if this method is called more than once and there will be
  // no side effect.
  bool ComputeBounds(int64 size);

 private:
  int64 first_byte_position_;
  int64 last_byte_position_;
  int64 suffix_length_;
  bool has_computed_bounds_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_BYTE_RANGE_H_
