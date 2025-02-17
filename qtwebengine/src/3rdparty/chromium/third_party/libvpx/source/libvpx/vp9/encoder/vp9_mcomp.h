/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_MCOMP_H_
#define VP9_ENCODER_VP9_MCOMP_H_

#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_variance.h"

#ifdef __cplusplus
extern "C" {
#endif

// The maximum number of steps in a step search given the largest
// allowed initial step
#define MAX_MVSEARCH_STEPS 11
// Max full pel mv specified in the unit of full pixel
// Enable the use of motion vector in range [-1023, 1023].
#define MAX_FULL_PEL_VAL ((1 << (MAX_MVSEARCH_STEPS - 1)) - 1)
// Maximum size of the first step in full pel units
#define MAX_FIRST_STEP (1 << (MAX_MVSEARCH_STEPS-1))
// Allowed motion vector pixel distance outside image border
// for Block_16x16
#define BORDER_MV_PIXELS_B16 (16 + VP9_INTERP_EXTEND)

// motion search site
typedef struct search_site {
  MV mv;
  int offset;
} search_site;

typedef struct search_site_config {
  search_site ss[8 * MAX_MVSEARCH_STEPS + 1];
  int ss_count;
  int searches_per_step;
} search_site_config;

void vp9_init_dsmotion_compensation(search_site_config *cfg, int stride);
void vp9_init3smotion_compensation(search_site_config *cfg,  int stride);

void vp9_set_mv_search_range(MACROBLOCK *x, const MV *mv);
int vp9_mv_bit_cost(const MV *mv, const MV *ref,
                    const int *mvjcost, int *mvcost[2], int weight);

// Utility to compute variance + MV rate cost for a given MV
int vp9_get_mvpred_var(const MACROBLOCK *x,
                       const MV *best_mv, const MV *center_mv,
                       const vp9_variance_fn_ptr_t *vfp,
                       int use_mvcost);
int vp9_get_mvpred_av_var(const MACROBLOCK *x,
                          const MV *best_mv, const MV *center_mv,
                          const uint8_t *second_pred,
                          const vp9_variance_fn_ptr_t *vfp,
                          int use_mvcost);

struct VP9_COMP;
struct SPEED_FEATURES;

int vp9_init_search_range(const struct SPEED_FEATURES *sf, int size);

// Runs sequence of diamond searches in smaller steps for RD
int vp9_full_pixel_diamond(const struct VP9_COMP *cpi, MACROBLOCK *x,
                           MV *mvp_full, int step_param,
                           int sadpb, int further_steps, int do_refine,
                           const vp9_variance_fn_ptr_t *fn_ptr,
                           const MV *ref_mv, MV *dst_mv);

typedef int (integer_mv_pattern_search_fn) (
    const MACROBLOCK *x,
    MV *ref_mv,
    int search_param,
    int error_per_bit,
    int do_init_search,
    const vp9_variance_fn_ptr_t *vf,
    int use_mvcost,
    const MV *center_mv,
    MV *best_mv);

integer_mv_pattern_search_fn vp9_hex_search;
integer_mv_pattern_search_fn vp9_bigdia_search;
integer_mv_pattern_search_fn vp9_square_search;
integer_mv_pattern_search_fn vp9_fast_hex_search;
integer_mv_pattern_search_fn vp9_fast_dia_search;

typedef int (fractional_mv_step_fp) (
    const MACROBLOCK *x,
    MV *bestmv, const MV *ref_mv,
    int allow_hp,
    int error_per_bit,
    const vp9_variance_fn_ptr_t *vfp,
    int forced_stop,  // 0 - full, 1 - qtr only, 2 - half only
    int iters_per_step,
    int *mvjcost,
    int *mvcost[2],
    int *distortion,
    unsigned int *sse);

extern fractional_mv_step_fp vp9_find_best_sub_pixel_tree;

typedef int (fractional_mv_step_comp_fp) (
    const MACROBLOCK *x,
    MV *bestmv, const MV *ref_mv,
    int allow_hp,
    int error_per_bit,
    const vp9_variance_fn_ptr_t *vfp,
    int forced_stop,  // 0 - full, 1 - qtr only, 2 - half only
    int iters_per_step,
    int *mvjcost, int *mvcost[2],
    int *distortion, unsigned int *sse1,
    const uint8_t *second_pred,
    int w, int h);

extern fractional_mv_step_comp_fp vp9_find_best_sub_pixel_comp_tree;

typedef int (*vp9_full_search_fn_t)(const MACROBLOCK *x,
                                    const MV *ref_mv, int sad_per_bit,
                                    int distance,
                                    const vp9_variance_fn_ptr_t *fn_ptr,
                                    const MV *center_mv, MV *best_mv);

typedef int (*vp9_refining_search_fn_t)(const MACROBLOCK *x,
                                        MV *ref_mv, int sad_per_bit,
                                        int distance,
                                        const vp9_variance_fn_ptr_t *fn_ptr,
                                        const MV *center_mv);

typedef int (*vp9_diamond_search_fn_t)(const MACROBLOCK *x,
                                       const search_site_config *cfg,
                                       MV *ref_mv, MV *best_mv,
                                       int search_param, int sad_per_bit,
                                       int *num00,
                                       const vp9_variance_fn_ptr_t *fn_ptr,
                                       const MV *center_mv);

int vp9_refining_search_8p_c(const MACROBLOCK *x,
                             MV *ref_mv, int error_per_bit,
                             int search_range,
                             const vp9_variance_fn_ptr_t *fn_ptr,
                             const MV *center_mv, const uint8_t *second_pred);
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_ENCODER_VP9_MCOMP_H_
