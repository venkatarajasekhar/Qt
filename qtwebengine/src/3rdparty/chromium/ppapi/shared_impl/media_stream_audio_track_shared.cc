// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/media_stream_audio_track_shared.h"

namespace ppapi {

// static
bool MediaStreamAudioTrackShared::VerifyAttributes(
    const Attributes& attributes) {
  if (attributes.buffers < 0)
    return false;
  return true;
}

}  // namespace ppapi
