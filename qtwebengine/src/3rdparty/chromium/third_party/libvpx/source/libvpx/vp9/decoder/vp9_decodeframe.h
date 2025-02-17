/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_DECODER_VP9_DECODEFRAME_H_
#define VP9_DECODER_VP9_DECODEFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct VP9Common;
struct VP9Decoder;

void vp9_init_dequantizer(struct VP9Common *cm);

int vp9_decode_frame(struct VP9Decoder *pbi,
                     const uint8_t *data, const uint8_t *data_end,
                     const uint8_t **p_data_end);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_DECODER_VP9_DECODEFRAME_H_
