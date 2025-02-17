/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_AQ_COMPLEXITY_H_
#define VP9_ENCODER_VP9_AQ_COMPLEXITY_H_

#ifdef __cplusplus
extern "C" {
#endif

struct VP9_COMP;

// Select a segment for the current SB64.
void vp9_select_in_frame_q_segment(struct VP9_COMP *cpi, int mi_row, int mi_col,
                                   int output_enabled, int projected_rate);


// This function sets up a set of segments with delta Q values around
// the baseline frame quantizer.
void vp9_setup_in_frame_q_adj(struct VP9_COMP *cpi);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_ENCODER_VP9_AQ_COMPLEXITY_H_
