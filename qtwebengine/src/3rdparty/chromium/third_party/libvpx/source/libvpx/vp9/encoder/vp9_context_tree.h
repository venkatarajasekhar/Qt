/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_CONTEXT_TREE_H_
#define VP9_ENCODER_VP9_CONTEXT_TREE_H_

#include "vp9/encoder/vp9_encoder.h"

void vp9_setup_pc_tree(VP9_COMMON *cm, MACROBLOCK *x);
void vp9_free_pc_tree(MACROBLOCK *x);

#endif /* VP9_ENCODER_VP9_CONTEXT_TREE_H_ */
