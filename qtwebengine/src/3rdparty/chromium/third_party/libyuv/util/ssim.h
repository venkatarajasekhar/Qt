/*
 *  Copyright 2013 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Get SSIM for video sequence. Assuming RAW 4:2:0 Y:Cb:Cr format

#ifndef UTIL_SSIM_H_  // NOLINT
#define UTIL_SSIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(INT_TYPES_DEFINED) && !defined(UINT8_TYPE_DEFINED)
typedef unsigned char uint8;
#define UINT8_TYPE_DEFINED
#endif

double CalcSSIM(const uint8* org, const uint8* rec,
                const int image_width, const int image_height);

// does -10.0 * log10(1.0 - ssim)
double CalcLSSIM(double ssim);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // UTIL_SSIM_H_  // NOLINT
