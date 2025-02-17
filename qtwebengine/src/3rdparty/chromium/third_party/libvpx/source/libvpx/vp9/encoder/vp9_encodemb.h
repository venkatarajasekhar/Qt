/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_ENCODEMB_H_
#define VP9_ENCODER_VP9_ENCODEMB_H_

#include "./vpx_config.h"
#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_encoder.h"
#include "vp9/common/vp9_onyxc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_encode_sb(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_encode_sby_pass1(MACROBLOCK *x, BLOCK_SIZE bsize);

void vp9_xform_quant(MACROBLOCK *x, int plane, int block,
                     BLOCK_SIZE plane_bsize, TX_SIZE tx_size);

void vp9_subtract_plane(MACROBLOCK *x, BLOCK_SIZE bsize, int plane);

void vp9_encode_block_intra(MACROBLOCK *x, int plane, int block,
                            BLOCK_SIZE plane_bsize, TX_SIZE tx_size,
                            unsigned char *skip);

void vp9_encode_intra_block_plane(MACROBLOCK *x, BLOCK_SIZE bsize, int plane);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_ENCODER_VP9_ENCODEMB_H_
