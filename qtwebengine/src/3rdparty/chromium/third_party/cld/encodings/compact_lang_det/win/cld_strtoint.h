// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ENCODINGS_COMPACT_LANG_DET_WIN_CLD_STRTOINT_H_
#define ENCODINGS_COMPACT_LANG_DET_WIN_CLD_STRTOINT_H_

#if !defined(CLD_WINDOWS)

//#include "cld/base/strtoint.h"

#else

#include <stdlib.h>

#include "encodings/compact_lang_det/win/cld_basictypes.h"

// This implementation is not as good as the one in base/strtoint.h,
// but it's sufficient for our purposes.
inline int32 strto32(const char *nptr, char **endptr, int base) {
  return static_cast<int32>(strtol(nptr, endptr, base));
}

#endif

#endif  // ENCODINGS_COMPACT_LANG_DET_WIN_CLD_STRTOINT_H_
