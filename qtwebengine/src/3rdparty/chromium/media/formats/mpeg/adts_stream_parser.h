// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_MPEG_ADTS_STREAM_PARSER_H_
#define MEDIA_FORMATS_MPEG_ADTS_STREAM_PARSER_H_

#include "base/basictypes.h"
#include "media/base/media_export.h"
#include "media/formats/mpeg/mpeg_audio_stream_parser_base.h"

namespace media {

class MEDIA_EXPORT ADTSStreamParser : public MPEGAudioStreamParserBase {
 public:
  ADTSStreamParser();
  virtual ~ADTSStreamParser();

 private:
  // MPEGAudioStreamParserBase overrides.
  virtual int ParseFrameHeader(const uint8* data,
                               int size,
                               int* frame_size,
                               int* sample_rate,
                               ChannelLayout* channel_layout,
                               int* sample_count,
                               bool* metadata_frame) const OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(ADTSStreamParser);
};

}  // namespace media

#endif  // MEDIA_FORMATS_MPEG_ADTS_STREAM_PARSER_H_
