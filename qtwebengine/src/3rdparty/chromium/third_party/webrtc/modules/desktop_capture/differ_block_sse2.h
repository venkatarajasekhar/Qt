/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This header file is used only differ_block.h. It defines the SSE2 rountines
// for finding block difference.

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_SSE2_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_SSE2_H_

#include <stdint.h>

namespace webrtc {

// Find block difference of dimension 16x16.
extern int BlockDifference_SSE2_W16(const uint8_t* image1,
                                    const uint8_t* image2,
                                    int stride);

// Find block difference of dimension 32x32.
extern int BlockDifference_SSE2_W32(const uint8_t* image1,
                                    const uint8_t* image2,
                                    int stride);

}  // namespace webrtc

#endif  // WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_BLOCK_SSE2_H_
