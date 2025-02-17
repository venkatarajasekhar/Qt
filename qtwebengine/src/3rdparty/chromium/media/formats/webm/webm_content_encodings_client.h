// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_WEBM_WEBM_CONTENT_ENCODINGS_CLIENT_H_
#define MEDIA_FORMATS_WEBM_WEBM_CONTENT_ENCODINGS_CLIENT_H_

#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "media/base/media_export.h"
#include "media/base/media_log.h"
#include "media/formats/webm/webm_content_encodings.h"
#include "media/formats/webm/webm_parser.h"

namespace media {

typedef std::vector<ContentEncoding*> ContentEncodings;

// Parser for WebM ContentEncodings element.
class MEDIA_EXPORT WebMContentEncodingsClient : public WebMParserClient {
 public:
  explicit WebMContentEncodingsClient(const LogCB& log_cb);
  virtual ~WebMContentEncodingsClient();

  const ContentEncodings& content_encodings() const;

  // WebMParserClient methods
  virtual WebMParserClient* OnListStart(int id) OVERRIDE;
  virtual bool OnListEnd(int id) OVERRIDE;
  virtual bool OnUInt(int id, int64 val) OVERRIDE;
  virtual bool OnBinary(int id, const uint8* data, int size) OVERRIDE;

 private:
  LogCB log_cb_;
  scoped_ptr<ContentEncoding> cur_content_encoding_;
  bool content_encryption_encountered_;
  ContentEncodings content_encodings_;

  // |content_encodings_| is ready. For debugging purpose.
  bool content_encodings_ready_;

  DISALLOW_COPY_AND_ASSIGN(WebMContentEncodingsClient);
};

}  // namespace media

#endif  // MEDIA_FORMATS_WEBM_WEBM_CONTENT_ENCODINGS_CLIENT_H_
