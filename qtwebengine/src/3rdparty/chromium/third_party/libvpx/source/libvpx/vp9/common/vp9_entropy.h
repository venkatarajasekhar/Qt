/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_ENTROPY_H_
#define VP9_COMMON_VP9_ENTROPY_H_

#include "vpx/vpx_integer.h"

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_scan.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIFF_UPDATE_PROB 252

// Coefficient token alphabet
#define ZERO_TOKEN      0   // 0     Extra Bits 0+0
#define ONE_TOKEN       1   // 1     Extra Bits 0+1
#define TWO_TOKEN       2   // 2     Extra Bits 0+1
#define THREE_TOKEN     3   // 3     Extra Bits 0+1
#define FOUR_TOKEN      4   // 4     Extra Bits 0+1
#define CATEGORY1_TOKEN 5   // 5-6   Extra Bits 1+1
#define CATEGORY2_TOKEN 6   // 7-10  Extra Bits 2+1
#define CATEGORY3_TOKEN 7   // 11-18 Extra Bits 3+1
#define CATEGORY4_TOKEN 8   // 19-34 Extra Bits 4+1
#define CATEGORY5_TOKEN 9   // 35-66 Extra Bits 5+1
#define CATEGORY6_TOKEN 10  // 67+   Extra Bits 14+1
#define EOB_TOKEN       11  // EOB   Extra Bits 0+0

#define ENTROPY_TOKENS 12

#define ENTROPY_NODES 11

DECLARE_ALIGNED(16, extern const uint8_t, vp9_pt_energy_class[ENTROPY_TOKENS]);

#define EOB_MODEL_TOKEN 3
extern const vp9_tree_index vp9_coefmodel_tree[];

typedef struct {
  const vp9_tree_index *tree;
  const vp9_prob *prob;
  int len;
  int base_val;
} vp9_extra_bit;

// indexed by token value
extern const vp9_extra_bit vp9_extra_bits[ENTROPY_TOKENS];

#define DCT_MAX_VALUE           16384

/* Coefficients are predicted via a 3-dimensional probability table. */

#define REF_TYPES 2  // intra=0, inter=1

/* Middle dimension reflects the coefficient position within the transform. */
#define COEF_BANDS 6

/* Inside dimension is measure of nearby complexity, that reflects the energy
   of nearby coefficients are nonzero.  For the first coefficient (DC, unless
   block type is 0), we look at the (already encoded) blocks above and to the
   left of the current block.  The context index is then the number (0,1,or 2)
   of these blocks having nonzero coefficients.
   After decoding a coefficient, the measure is determined by the size of the
   most recently decoded coefficient.
   Note that the intuitive meaning of this measure changes as coefficients
   are decoded, e.g., prior to the first token, a zero means that my neighbors
   are empty while, after the first token, because of the use of end-of-block,
   a zero means we just decoded a zero and hence guarantees that a non-zero
   coefficient will appear later in this block.  However, this shift
   in meaning is perfectly OK because our context depends also on the
   coefficient band (and since zigzag positions 0, 1, and 2 are in
   distinct bands). */

#define COEFF_CONTEXTS 6
#define BAND_COEFF_CONTEXTS(band) ((band) == 0 ? 3 : COEFF_CONTEXTS)

// #define ENTROPY_STATS

typedef unsigned int vp9_coeff_count[REF_TYPES][COEF_BANDS][COEFF_CONTEXTS]
                                    [ENTROPY_TOKENS];
typedef unsigned int vp9_coeff_stats[REF_TYPES][COEF_BANDS][COEFF_CONTEXTS]
                                    [ENTROPY_NODES][2];

#define SUBEXP_PARAM                4   /* Subexponential code parameter */
#define MODULUS_PARAM               13  /* Modulus parameter */

struct VP9Common;
void vp9_default_coef_probs(struct VP9Common *cm);
void vp9_adapt_coef_probs(struct VP9Common *cm);

static INLINE void reset_skip_context(MACROBLOCKD *xd, BLOCK_SIZE bsize) {
  int i;
  for (i = 0; i < MAX_MB_PLANE; i++) {
    struct macroblockd_plane *const pd = &xd->plane[i];
    const BLOCK_SIZE plane_bsize = get_plane_block_size(bsize, pd);
    vpx_memset(pd->above_context, 0, sizeof(ENTROPY_CONTEXT) *
                   num_4x4_blocks_wide_lookup[plane_bsize]);
    vpx_memset(pd->left_context, 0, sizeof(ENTROPY_CONTEXT) *
                   num_4x4_blocks_high_lookup[plane_bsize]);
  }
}

// This is the index in the scan order beyond which all coefficients for
// 8x8 transform and above are in the top band.
// This macro is currently unused but may be used by certain implementations
#define MAXBAND_INDEX 21

DECLARE_ALIGNED(16, extern const uint8_t, vp9_coefband_trans_8x8plus[1024]);
DECLARE_ALIGNED(16, extern const uint8_t, vp9_coefband_trans_4x4[16]);

static INLINE const uint8_t *get_band_translate(TX_SIZE tx_size) {
  return tx_size == TX_4X4 ? vp9_coefband_trans_4x4
                           : vp9_coefband_trans_8x8plus;
}

// 128 lists of probabilities are stored for the following ONE node probs:
// 1, 3, 5, 7, ..., 253, 255
// In between probabilities are interpolated linearly

#define COEFF_PROB_MODELS 256

#define UNCONSTRAINED_NODES         3

#define PIVOT_NODE                  2   // which node is pivot

#define MODEL_NODES (ENTROPY_NODES - UNCONSTRAINED_NODES)
extern const vp9_prob vp9_pareto8_full[COEFF_PROB_MODELS][MODEL_NODES];

typedef vp9_prob vp9_coeff_probs_model[REF_TYPES][COEF_BANDS]
                                      [COEFF_CONTEXTS][UNCONSTRAINED_NODES];

typedef unsigned int vp9_coeff_count_model[REF_TYPES][COEF_BANDS]
                                          [COEFF_CONTEXTS]
                                          [UNCONSTRAINED_NODES + 1];

void vp9_model_to_full_probs(const vp9_prob *model, vp9_prob *full);

static INLINE int get_entropy_context(TX_SIZE tx_size, const ENTROPY_CONTEXT *a,
                                      const ENTROPY_CONTEXT *l) {
  ENTROPY_CONTEXT above_ec = 0, left_ec = 0;

  switch (tx_size) {
    case TX_4X4:
      above_ec = a[0] != 0;
      left_ec = l[0] != 0;
      break;
    case TX_8X8:
      above_ec = !!*(const uint16_t *)a;
      left_ec  = !!*(const uint16_t *)l;
      break;
    case TX_16X16:
      above_ec = !!*(const uint32_t *)a;
      left_ec  = !!*(const uint32_t *)l;
      break;
    case TX_32X32:
      above_ec = !!*(const uint64_t *)a;
      left_ec  = !!*(const uint64_t *)l;
      break;
    default:
      assert(0 && "Invalid transform size.");
  }

  return combine_entropy_contexts(above_ec, left_ec);
}

static const INLINE scan_order *get_scan(const MACROBLOCKD *xd, TX_SIZE tx_size,
                                         PLANE_TYPE type, int block_idx) {
  const MODE_INFO *const mi = xd->mi[0];

  if (is_inter_block(&mi->mbmi) || type != PLANE_TYPE_Y || xd->lossless) {
    return &vp9_default_scan_orders[tx_size];
  } else {
    const PREDICTION_MODE mode = get_y_mode(mi, block_idx);
    return &vp9_scan_orders[tx_size][intra_mode_to_tx_type_lookup[mode]];
  }
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_COMMON_VP9_ENTROPY_H_
