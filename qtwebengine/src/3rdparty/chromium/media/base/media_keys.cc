// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/media_keys.h"

namespace media {

MediaKeys::MediaKeys() {}

MediaKeys::~MediaKeys() {}

Decryptor* MediaKeys::GetDecryptor() { return NULL; }

}  // namespace media
