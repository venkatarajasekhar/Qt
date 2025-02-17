/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_ENTROPYMODE_H_
#define VP9_COMMON_VP9_ENTROPYMODE_H_

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/common/vp9_entropymv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TX_SIZE_CONTEXTS 2
#define SWITCHABLE_FILTERS 3   // number of switchable filters
#define SWITCHABLE_FILTER_CONTEXTS (SWITCHABLE_FILTERS + 1)

struct VP9Common;

struct tx_probs {
  vp9_prob p32x32[TX_SIZE_CONTEXTS][TX_SIZES - 1];
  vp9_prob p16x16[TX_SIZE_CONTEXTS][TX_SIZES - 2];
  vp9_prob p8x8[TX_SIZE_CONTEXTS][TX_SIZES - 3];
};

struct tx_counts {
  unsigned int p32x32[TX_SIZE_CONTEXTS][TX_SIZES];
  unsigned int p16x16[TX_SIZE_CONTEXTS][TX_SIZES - 1];
  unsigned int p8x8[TX_SIZE_CONTEXTS][TX_SIZES - 2];
};

typedef struct frame_contexts {
  vp9_prob y_mode_prob[BLOCK_SIZE_GROUPS][INTRA_MODES - 1];
  vp9_prob uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];
  vp9_prob partition_prob[PARTITION_CONTEXTS][PARTITION_TYPES - 1];
  vp9_coeff_probs_model coef_probs[TX_SIZES][PLANE_TYPES];
  vp9_prob switchable_interp_prob[SWITCHABLE_FILTER_CONTEXTS]
                                 [SWITCHABLE_FILTERS - 1];
  vp9_prob inter_mode_probs[INTER_MODE_CONTEXTS][INTER_MODES - 1];
  vp9_prob intra_inter_prob[INTRA_INTER_CONTEXTS];
  vp9_prob comp_inter_prob[COMP_INTER_CONTEXTS];
  vp9_prob single_ref_prob[REF_CONTEXTS][2];
  vp9_prob comp_ref_prob[REF_CONTEXTS];
  struct tx_probs tx_probs;
  vp9_prob skip_probs[SKIP_CONTEXTS];
  nmv_context nmvc;
} FRAME_CONTEXT;

typedef struct {
  unsigned int y_mode[BLOCK_SIZE_GROUPS][INTRA_MODES];
  unsigned int uv_mode[INTRA_MODES][INTRA_MODES];
  unsigned int partition[PARTITION_CONTEXTS][PARTITION_TYPES];
  vp9_coeff_count_model coef[TX_SIZES][PLANE_TYPES];
  unsigned int eob_branch[TX_SIZES][PLANE_TYPES][REF_TYPES]
                         [COEF_BANDS][COEFF_CONTEXTS];
  unsigned int switchable_interp[SWITCHABLE_FILTER_CONTEXTS]
                                [SWITCHABLE_FILTERS];
  unsigned int inter_mode[INTER_MODE_CONTEXTS][INTER_MODES];
  unsigned int intra_inter[INTRA_INTER_CONTEXTS][2];
  unsigned int comp_inter[COMP_INTER_CONTEXTS][2];
  unsigned int single_ref[REF_CONTEXTS][2][2];
  unsigned int comp_ref[REF_CONTEXTS][2];
  struct tx_counts tx;
  unsigned int skip[SKIP_CONTEXTS][2];
  nmv_context_counts mv;
} FRAME_COUNTS;

extern const vp9_prob vp9_kf_uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];
extern const vp9_prob vp9_kf_y_mode_prob[INTRA_MODES][INTRA_MODES]
                                        [INTRA_MODES - 1];
extern const vp9_prob vp9_kf_partition_probs[PARTITION_CONTEXTS]
                                            [PARTITION_TYPES - 1];
extern const vp9_tree_index vp9_intra_mode_tree[TREE_SIZE(INTRA_MODES)];
extern const vp9_tree_index vp9_inter_mode_tree[TREE_SIZE(INTER_MODES)];
extern const vp9_tree_index vp9_partition_tree[TREE_SIZE(PARTITION_TYPES)];
extern const vp9_tree_index vp9_switchable_interp_tree
                                [TREE_SIZE(SWITCHABLE_FILTERS)];

void vp9_setup_past_independence(struct VP9Common *cm);

void vp9_init_mode_probs(FRAME_CONTEXT *fc);

void vp9_adapt_mode_probs(struct VP9Common *cm);

void tx_counts_to_branch_counts_32x32(const unsigned int *tx_count_32x32p,
                                      unsigned int (*ct_32x32p)[2]);
void tx_counts_to_branch_counts_16x16(const unsigned int *tx_count_16x16p,
                                      unsigned int (*ct_16x16p)[2]);
void tx_counts_to_branch_counts_8x8(const unsigned int *tx_count_8x8p,
                                    unsigned int (*ct_8x8p)[2]);

static INLINE const vp9_prob *get_y_mode_probs(const MODE_INFO *mi,
                                               const MODE_INFO *above_mi,
                                               const MODE_INFO *left_mi,
                                               int block) {
  const PREDICTION_MODE above = vp9_above_block_mode(mi, above_mi, block);
  const PREDICTION_MODE left = vp9_left_block_mode(mi, left_mi, block);
  return vp9_kf_y_mode_prob[above][left];
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_COMMON_VP9_ENTROPYMODE_H_
