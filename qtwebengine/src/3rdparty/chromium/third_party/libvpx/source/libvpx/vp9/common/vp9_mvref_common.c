
/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_mvref_common.h"

#define MVREF_NEIGHBOURS 8

typedef struct position {
  int row;
  int col;
} POSITION;

typedef enum {
  BOTH_ZERO = 0,
  ZERO_PLUS_PREDICTED = 1,
  BOTH_PREDICTED = 2,
  NEW_PLUS_NON_INTRA = 3,
  BOTH_NEW = 4,
  INTRA_PLUS_NON_INTRA = 5,
  BOTH_INTRA = 6,
  INVALID_CASE = 9
} motion_vector_context;

// This is used to figure out a context for the ref blocks. The code flattens
// an array that would have 3 possible counts (0, 1 & 2) for 3 choices by
// adding 9 for each intra block, 3 for each zero mv and 1 for each new
// motion vector. This single number is then converted into a context
// with a single lookup ( counter_to_context ).
static const int mode_2_counter[MB_MODE_COUNT] = {
  9,  // DC_PRED
  9,  // V_PRED
  9,  // H_PRED
  9,  // D45_PRED
  9,  // D135_PRED
  9,  // D117_PRED
  9,  // D153_PRED
  9,  // D207_PRED
  9,  // D63_PRED
  9,  // TM_PRED
  0,  // NEARESTMV
  0,  // NEARMV
  3,  // ZEROMV
  1,  // NEWMV
};

// There are 3^3 different combinations of 3 counts that can be either 0,1 or
// 2. However the actual count can never be greater than 2 so the highest
// counter we need is 18. 9 is an invalid counter that's never used.
static const int counter_to_context[19] = {
  BOTH_PREDICTED,  // 0
  NEW_PLUS_NON_INTRA,  // 1
  BOTH_NEW,  // 2
  ZERO_PLUS_PREDICTED,  // 3
  NEW_PLUS_NON_INTRA,  // 4
  INVALID_CASE,  // 5
  BOTH_ZERO,  // 6
  INVALID_CASE,  // 7
  INVALID_CASE,  // 8
  INTRA_PLUS_NON_INTRA,  // 9
  INTRA_PLUS_NON_INTRA,  // 10
  INVALID_CASE,  // 11
  INTRA_PLUS_NON_INTRA,  // 12
  INVALID_CASE,  // 13
  INVALID_CASE,  // 14
  INVALID_CASE,  // 15
  INVALID_CASE,  // 16
  INVALID_CASE,  // 17
  BOTH_INTRA  // 18
};

static const POSITION mv_ref_blocks[BLOCK_SIZES][MVREF_NEIGHBOURS] = {
  // 4X4
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  // 4X8
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  // 8X4
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  // 8X8
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  // 8X16
  {{0, -1}, {-1, 0}, {1, -1}, {-1, -1}, {0, -2}, {-2, 0}, {-2, -1}, {-1, -2}},
  // 16X8
  {{-1, 0}, {0, -1}, {-1, 1}, {-1, -1}, {-2, 0}, {0, -2}, {-1, -2}, {-2, -1}},
  // 16X16
  {{-1, 0}, {0, -1}, {-1, 1}, {1, -1}, {-1, -1}, {-3, 0}, {0, -3}, {-3, -3}},
  // 16X32
  {{0, -1}, {-1, 0}, {2, -1}, {-1, -1}, {-1, 1}, {0, -3}, {-3, 0}, {-3, -3}},
  // 32X16
  {{-1, 0}, {0, -1}, {-1, 2}, {-1, -1}, {1, -1}, {-3, 0}, {0, -3}, {-3, -3}},
  // 32X32
  {{-1, 1}, {1, -1}, {-1, 2}, {2, -1}, {-1, -1}, {-3, 0}, {0, -3}, {-3, -3}},
  // 32X64
  {{0, -1}, {-1, 0}, {4, -1}, {-1, 2}, {-1, -1}, {0, -3}, {-3, 0}, {2, -1}},
  // 64X32
  {{-1, 0}, {0, -1}, {-1, 4}, {2, -1}, {-1, -1}, {-3, 0}, {0, -3}, {-1, 2}},
  // 64X64
  {{-1, 3}, {3, -1}, {-1, 4}, {4, -1}, {-1, -1}, {-1, 0}, {0, -1}, {-1, 6}}
};

static const int idx_n_column_to_subblock[4][2] = {
  {1, 2},
  {1, 3},
  {3, 2},
  {3, 3}
};

// clamp_mv_ref
#define MV_BORDER (16 << 3)  // Allow 16 pels in 1/8th pel units

static void clamp_mv_ref(MV *mv, const MACROBLOCKD *xd) {
  clamp_mv(mv, xd->mb_to_left_edge - MV_BORDER,
               xd->mb_to_right_edge + MV_BORDER,
               xd->mb_to_top_edge - MV_BORDER,
               xd->mb_to_bottom_edge + MV_BORDER);
}

// This function returns either the appropriate sub block or block's mv
// on whether the block_size < 8x8 and we have check_sub_blocks set.
static INLINE int_mv get_sub_block_mv(const MODE_INFO *candidate, int which_mv,
                                      int search_col, int block_idx) {
  return block_idx >= 0 && candidate->mbmi.sb_type < BLOCK_8X8
          ? candidate->bmi[idx_n_column_to_subblock[block_idx][search_col == 0]]
              .as_mv[which_mv]
          : candidate->mbmi.mv[which_mv];
}


// Performs mv sign inversion if indicated by the reference frame combination.
static INLINE int_mv scale_mv(const MB_MODE_INFO *mbmi, int ref,
                              const MV_REFERENCE_FRAME this_ref_frame,
                              const int *ref_sign_bias) {
  int_mv mv = mbmi->mv[ref];
  if (ref_sign_bias[mbmi->ref_frame[ref]] != ref_sign_bias[this_ref_frame]) {
    mv.as_mv.row *= -1;
    mv.as_mv.col *= -1;
  }
  return mv;
}

// This macro is used to add a motion vector mv_ref list if it isn't
// already in the list.  If it's the second motion vector it will also
// skip all additional processing and jump to done!
#define ADD_MV_REF_LIST(mv) \
  do { \
    if (refmv_count) { \
      if ((mv).as_int != mv_ref_list[0].as_int) { \
        mv_ref_list[refmv_count] = (mv); \
        goto Done; \
      } \
    } else { \
      mv_ref_list[refmv_count++] = (mv); \
    } \
  } while (0)

// If either reference frame is different, not INTRA, and they
// are different from each other scale and add the mv to our list.
#define IF_DIFF_REF_FRAME_ADD_MV(mbmi) \
  do { \
    if (is_inter_block(mbmi)) { \
      if ((mbmi)->ref_frame[0] != ref_frame) \
        ADD_MV_REF_LIST(scale_mv((mbmi), 0, ref_frame, ref_sign_bias)); \
      if (has_second_ref(mbmi) && \
          (mbmi)->ref_frame[1] != ref_frame && \
          (mbmi)->mv[1].as_int != (mbmi)->mv[0].as_int) \
        ADD_MV_REF_LIST(scale_mv((mbmi), 1, ref_frame, ref_sign_bias)); \
    } \
  } while (0)


// Checks that the given mi_row, mi_col and search point
// are inside the borders of the tile.
static INLINE int is_inside(const TileInfo *const tile,
                            int mi_col, int mi_row, int mi_rows,
                            const POSITION *mi_pos) {
  return !(mi_row + mi_pos->row < 0 ||
           mi_col + mi_pos->col < tile->mi_col_start ||
           mi_row + mi_pos->row >= mi_rows ||
           mi_col + mi_pos->col >= tile->mi_col_end);
}

// This function searches the neighbourhood of a given MB/SB
// to try and find candidate reference vectors.
static void find_mv_refs_idx(const VP9_COMMON *cm, const MACROBLOCKD *xd,
                             const TileInfo *const tile,
                             MODE_INFO *mi, MV_REFERENCE_FRAME ref_frame,
                             int_mv *mv_ref_list,
                             int block, int mi_row, int mi_col) {
  const int *ref_sign_bias = cm->ref_frame_sign_bias;
  int i, refmv_count = 0;
  const MODE_INFO *prev_mi = cm->coding_use_prev_mi && cm->prev_mi
        ? cm->prev_mi_grid_visible[mi_row * xd->mi_stride + mi_col]
        : NULL;
  const MB_MODE_INFO *const prev_mbmi = prev_mi ? &prev_mi->mbmi : NULL;


  const POSITION *const mv_ref_search = mv_ref_blocks[mi->mbmi.sb_type];

  int different_ref_found = 0;
  int context_counter = 0;

  // Blank the reference vector list
  vpx_memset(mv_ref_list, 0, sizeof(*mv_ref_list) * MAX_MV_REF_CANDIDATES);

  // The nearest 2 blocks are treated differently
  // if the size < 8x8 we get the mv from the bmi substructure,
  // and we also need to keep a mode count.
  for (i = 0; i < 2; ++i) {
    const POSITION *const mv_ref = &mv_ref_search[i];
    if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
      const MODE_INFO *const candidate_mi = xd->mi[mv_ref->col + mv_ref->row *
                                                   xd->mi_stride];
      const MB_MODE_INFO *const candidate = &candidate_mi->mbmi;
      // Keep counts for entropy encoding.
      context_counter += mode_2_counter[candidate->mode];
      different_ref_found = 1;

      if (candidate->ref_frame[0] == ref_frame)
        ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 0, mv_ref->col, block));
      else if (candidate->ref_frame[1] == ref_frame)
        ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 1, mv_ref->col, block));
    }
  }

  // Check the rest of the neighbors in much the same way
  // as before except we don't need to keep track of sub blocks or
  // mode counts.
  for (; i < MVREF_NEIGHBOURS; ++i) {
    const POSITION *const mv_ref = &mv_ref_search[i];
    if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
      const MB_MODE_INFO *const candidate = &xd->mi[mv_ref->col + mv_ref->row *
                                                    xd->mi_stride]->mbmi;
      different_ref_found = 1;

      if (candidate->ref_frame[0] == ref_frame)
        ADD_MV_REF_LIST(candidate->mv[0]);
      else if (candidate->ref_frame[1] == ref_frame)
        ADD_MV_REF_LIST(candidate->mv[1]);
    }
  }

  // Check the last frame's mode and mv info.
  if (prev_mbmi) {
    if (prev_mbmi->ref_frame[0] == ref_frame)
      ADD_MV_REF_LIST(prev_mbmi->mv[0]);
    else if (prev_mbmi->ref_frame[1] == ref_frame)
      ADD_MV_REF_LIST(prev_mbmi->mv[1]);
  }

  // Since we couldn't find 2 mvs from the same reference frame
  // go back through the neighbors and find motion vectors from
  // different reference frames.
  if (different_ref_found) {
    for (i = 0; i < MVREF_NEIGHBOURS; ++i) {
      const POSITION *mv_ref = &mv_ref_search[i];
      if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
        const MB_MODE_INFO *const candidate = &xd->mi[mv_ref->col + mv_ref->row
                                              * xd->mi_stride]->mbmi;

        // If the candidate is INTRA we don't want to consider its mv.
        IF_DIFF_REF_FRAME_ADD_MV(candidate);
      }
    }
  }

  // Since we still don't have a candidate we'll try the last frame.
  if (prev_mbmi)
    IF_DIFF_REF_FRAME_ADD_MV(prev_mbmi);

 Done:

  mi->mbmi.mode_context[ref_frame] = counter_to_context[context_counter];

  // Clamp vectors
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i)
    clamp_mv_ref(&mv_ref_list[i].as_mv, xd);
}

void vp9_find_mv_refs(const VP9_COMMON *cm, const MACROBLOCKD *xd,
                                    const TileInfo *const tile,
                                    MODE_INFO *mi, MV_REFERENCE_FRAME ref_frame,
                                    int_mv *mv_ref_list,
                                    int mi_row, int mi_col) {
  find_mv_refs_idx(cm, xd, tile, mi, ref_frame, mv_ref_list, -1,
                   mi_row, mi_col);
}

static void lower_mv_precision(MV *mv, int allow_hp) {
  const int use_hp = allow_hp && vp9_use_mv_hp(mv);
  if (!use_hp) {
    if (mv->row & 1)
      mv->row += (mv->row > 0 ? -1 : 1);
    if (mv->col & 1)
      mv->col += (mv->col > 0 ? -1 : 1);
  }
}


void vp9_find_best_ref_mvs(MACROBLOCKD *xd, int allow_hp,
                           int_mv *mvlist, int_mv *nearest, int_mv *near) {
  int i;
  // Make sure all the candidates are properly clamped etc
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
    lower_mv_precision(&mvlist[i].as_mv, allow_hp);
    clamp_mv2(&mvlist[i].as_mv, xd);
  }
  *nearest = mvlist[0];
  *near = mvlist[1];
}

void vp9_append_sub8x8_mvs_for_idx(VP9_COMMON *cm, MACROBLOCKD *xd,
                                   const TileInfo *const tile,
                                   int block, int ref, int mi_row, int mi_col,
                                   int_mv *nearest, int_mv *near) {
  int_mv mv_list[MAX_MV_REF_CANDIDATES];
  MODE_INFO *const mi = xd->mi[0];
  b_mode_info *bmi = mi->bmi;
  int n;

  assert(MAX_MV_REF_CANDIDATES == 2);

  find_mv_refs_idx(cm, xd, tile, mi, mi->mbmi.ref_frame[ref], mv_list, block,
                   mi_row, mi_col);

  near->as_int = 0;
  switch (block) {
    case 0:
      nearest->as_int = mv_list[0].as_int;
      near->as_int = mv_list[1].as_int;
      break;
    case 1:
    case 2:
      nearest->as_int = bmi[0].as_mv[ref].as_int;
      for (n = 0; n < MAX_MV_REF_CANDIDATES; ++n)
        if (nearest->as_int != mv_list[n].as_int) {
          near->as_int = mv_list[n].as_int;
          break;
        }
      break;
    case 3: {
      int_mv candidates[2 + MAX_MV_REF_CANDIDATES];
      candidates[0] = bmi[1].as_mv[ref];
      candidates[1] = bmi[0].as_mv[ref];
      candidates[2] = mv_list[0];
      candidates[3] = mv_list[1];

      nearest->as_int = bmi[2].as_mv[ref].as_int;
      for (n = 0; n < 2 + MAX_MV_REF_CANDIDATES; ++n)
        if (nearest->as_int != candidates[n].as_int) {
          near->as_int = candidates[n].as_int;
          break;
        }
      break;
    }
    default:
      assert("Invalid block index.");
  }
}
