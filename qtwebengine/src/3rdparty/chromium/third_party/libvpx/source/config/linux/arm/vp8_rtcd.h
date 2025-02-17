#ifndef VP8_RTCD_H_
#define VP8_RTCD_H_

#ifdef RTCD_C
#define RTCD_EXTERN
#else
#define RTCD_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * VP8
 */

struct blockd;
struct macroblockd;
struct loop_filter_info;

/* Encoder forward decls */
struct block;
struct macroblock;
struct variance_vtable;
union int_mv;
struct yv12_buffer_config;

void vp8_bilinear_predict16x16_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_bilinear_predict16x16_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_bilinear_predict16x16 vp8_bilinear_predict16x16_armv6

void vp8_bilinear_predict4x4_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_bilinear_predict4x4_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_bilinear_predict4x4 vp8_bilinear_predict4x4_armv6

void vp8_bilinear_predict8x4_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_bilinear_predict8x4_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_bilinear_predict8x4 vp8_bilinear_predict8x4_armv6

void vp8_bilinear_predict8x8_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_bilinear_predict8x8_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_bilinear_predict8x8 vp8_bilinear_predict8x8_armv6

void vp8_blend_b_c(unsigned char *y, unsigned char *u, unsigned char *v, int y1, int u1, int v1, int alpha, int stride);
#define vp8_blend_b vp8_blend_b_c

void vp8_blend_mb_inner_c(unsigned char *y, unsigned char *u, unsigned char *v, int y1, int u1, int v1, int alpha, int stride);
#define vp8_blend_mb_inner vp8_blend_mb_inner_c

void vp8_blend_mb_outer_c(unsigned char *y, unsigned char *u, unsigned char *v, int y1, int u1, int v1, int alpha, int stride);
#define vp8_blend_mb_outer vp8_blend_mb_outer_c

int vp8_block_error_c(short *coeff, short *dqcoeff);
#define vp8_block_error vp8_block_error_c

void vp8_build_intra_predictors_mbuv_s_c(struct macroblockd *x, unsigned char * uabove_row, unsigned char * vabove_row,  unsigned char *uleft, unsigned char *vleft, int left_stride, unsigned char * upred_ptr, unsigned char * vpred_ptr, int pred_stride);
#define vp8_build_intra_predictors_mbuv_s vp8_build_intra_predictors_mbuv_s_c

void vp8_build_intra_predictors_mby_s_c(struct macroblockd *x, unsigned char * yabove_row, unsigned char * yleft, int left_stride, unsigned char * ypred_ptr, int y_stride);
#define vp8_build_intra_predictors_mby_s vp8_build_intra_predictors_mby_s_c

void vp8_clear_system_state_c();
#define vp8_clear_system_state vp8_clear_system_state_c

void vp8_copy_mem16x16_c(unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch);
void vp8_copy_mem16x16_v6(unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch);
#define vp8_copy_mem16x16 vp8_copy_mem16x16_v6

void vp8_copy_mem8x4_c(unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch);
void vp8_copy_mem8x4_v6(unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch);
#define vp8_copy_mem8x4 vp8_copy_mem8x4_v6

void vp8_copy_mem8x8_c(unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch);
void vp8_copy_mem8x8_v6(unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch);
#define vp8_copy_mem8x8 vp8_copy_mem8x8_v6

void vp8_dc_only_idct_add_c(short input, unsigned char *pred, int pred_stride, unsigned char *dst, int dst_stride);
void vp8_dc_only_idct_add_v6(short input, unsigned char *pred, int pred_stride, unsigned char *dst, int dst_stride);
#define vp8_dc_only_idct_add vp8_dc_only_idct_add_v6

int vp8_denoiser_filter_c(unsigned char *mc_running_avg_y, int mc_avg_y_stride, unsigned char *running_avg_y, int avg_y_stride, unsigned char *sig, int sig_stride, unsigned int motion_magnitude, int increase_denoising);
#define vp8_denoiser_filter vp8_denoiser_filter_c

void vp8_dequant_idct_add_c(short *input, short *dq, unsigned char *output, int stride);
void vp8_dequant_idct_add_v6(short *input, short *dq, unsigned char *output, int stride);
#define vp8_dequant_idct_add vp8_dequant_idct_add_v6

void vp8_dequant_idct_add_uv_block_c(short *q, short *dq, unsigned char *dst_u, unsigned char *dst_v, int stride, char *eobs);
void vp8_dequant_idct_add_uv_block_v6(short *q, short *dq, unsigned char *dst_u, unsigned char *dst_v, int stride, char *eobs);
#define vp8_dequant_idct_add_uv_block vp8_dequant_idct_add_uv_block_v6

void vp8_dequant_idct_add_y_block_c(short *q, short *dq, unsigned char *dst, int stride, char *eobs);
void vp8_dequant_idct_add_y_block_v6(short *q, short *dq, unsigned char *dst, int stride, char *eobs);
#define vp8_dequant_idct_add_y_block vp8_dequant_idct_add_y_block_v6

void vp8_dequantize_b_c(struct blockd*, short *dqc);
void vp8_dequantize_b_v6(struct blockd*, short *dqc);
#define vp8_dequantize_b vp8_dequantize_b_v6

int vp8_diamond_search_sad_c(struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, union int_mv *best_mv, int search_param, int sad_per_bit, int *num00, struct variance_vtable *fn_ptr, int *mvcost[2], union int_mv *center_mv);
#define vp8_diamond_search_sad vp8_diamond_search_sad_c

void vp8_fast_quantize_b_c(struct block *, struct blockd *);
void vp8_fast_quantize_b_armv6(struct block *, struct blockd *);
#define vp8_fast_quantize_b vp8_fast_quantize_b_armv6

void vp8_fast_quantize_b_pair_c(struct block *b1, struct block *b2, struct blockd *d1, struct blockd *d2);
#define vp8_fast_quantize_b_pair vp8_fast_quantize_b_pair_c

void vp8_filter_by_weight16x16_c(unsigned char *src, int src_stride, unsigned char *dst, int dst_stride, int src_weight);
#define vp8_filter_by_weight16x16 vp8_filter_by_weight16x16_c

void vp8_filter_by_weight4x4_c(unsigned char *src, int src_stride, unsigned char *dst, int dst_stride, int src_weight);
#define vp8_filter_by_weight4x4 vp8_filter_by_weight4x4_c

void vp8_filter_by_weight8x8_c(unsigned char *src, int src_stride, unsigned char *dst, int dst_stride, int src_weight);
#define vp8_filter_by_weight8x8 vp8_filter_by_weight8x8_c

int vp8_full_search_sad_c(struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, int sad_per_bit, int distance, struct variance_vtable *fn_ptr, int *mvcost[2], union int_mv *center_mv);
#define vp8_full_search_sad vp8_full_search_sad_c

unsigned int vp8_get4x4sse_cs_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride);
#define vp8_get4x4sse_cs vp8_get4x4sse_cs_c

unsigned int vp8_get_mb_ss_c(const short *);
#define vp8_get_mb_ss vp8_get_mb_ss_c

void vp8_intra4x4_predict_c(unsigned char *Above, unsigned char *yleft, int left_stride, int b_mode, unsigned char *dst, int dst_stride, unsigned char top_left);
void vp8_intra4x4_predict_armv6(unsigned char *Above, unsigned char *yleft, int left_stride, int b_mode, unsigned char *dst, int dst_stride, unsigned char top_left);
#define vp8_intra4x4_predict vp8_intra4x4_predict_armv6

void vp8_loop_filter_bh_c(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
void vp8_loop_filter_bh_armv6(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
#define vp8_loop_filter_bh vp8_loop_filter_bh_armv6

void vp8_loop_filter_bv_c(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
void vp8_loop_filter_bv_armv6(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
#define vp8_loop_filter_bv vp8_loop_filter_bv_armv6

void vp8_loop_filter_mbh_c(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
void vp8_loop_filter_mbh_armv6(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
#define vp8_loop_filter_mbh vp8_loop_filter_mbh_armv6

void vp8_loop_filter_mbv_c(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
void vp8_loop_filter_mbv_armv6(unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi);
#define vp8_loop_filter_mbv vp8_loop_filter_mbv_armv6

void vp8_loop_filter_bhs_c(unsigned char *y, int ystride, const unsigned char *blimit);
void vp8_loop_filter_bhs_armv6(unsigned char *y, int ystride, const unsigned char *blimit);
#define vp8_loop_filter_simple_bh vp8_loop_filter_bhs_armv6

void vp8_loop_filter_bvs_c(unsigned char *y, int ystride, const unsigned char *blimit);
void vp8_loop_filter_bvs_armv6(unsigned char *y, int ystride, const unsigned char *blimit);
#define vp8_loop_filter_simple_bv vp8_loop_filter_bvs_armv6

void vp8_loop_filter_simple_horizontal_edge_c(unsigned char *y, int ystride, const unsigned char *blimit);
void vp8_loop_filter_simple_horizontal_edge_armv6(unsigned char *y, int ystride, const unsigned char *blimit);
#define vp8_loop_filter_simple_mbh vp8_loop_filter_simple_horizontal_edge_armv6

void vp8_loop_filter_simple_vertical_edge_c(unsigned char *y, int ystride, const unsigned char *blimit);
void vp8_loop_filter_simple_vertical_edge_armv6(unsigned char *y, int ystride, const unsigned char *blimit);
#define vp8_loop_filter_simple_mbv vp8_loop_filter_simple_vertical_edge_armv6

int vp8_mbblock_error_c(struct macroblock *mb, int dc);
#define vp8_mbblock_error vp8_mbblock_error_c

void vp8_mbpost_proc_across_ip_c(unsigned char *dst, int pitch, int rows, int cols,int flimit);
#define vp8_mbpost_proc_across_ip vp8_mbpost_proc_across_ip_c

void vp8_mbpost_proc_down_c(unsigned char *dst, int pitch, int rows, int cols,int flimit);
#define vp8_mbpost_proc_down vp8_mbpost_proc_down_c

int vp8_mbuverror_c(struct macroblock *mb);
#define vp8_mbuverror vp8_mbuverror_c

unsigned int vp8_mse16x16_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
unsigned int vp8_mse16x16_armv6(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_mse16x16 vp8_mse16x16_armv6

void vp8_plane_add_noise_c(unsigned char *s, char *noise, char blackclamp[16], char whiteclamp[16], char bothclamp[16], unsigned int w, unsigned int h, int pitch);
#define vp8_plane_add_noise vp8_plane_add_noise_c

void vp8_post_proc_down_and_across_mb_row_c(unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, int cols, unsigned char *flimits, int size);
#define vp8_post_proc_down_and_across_mb_row vp8_post_proc_down_and_across_mb_row_c

void vp8_quantize_mb_c(struct macroblock *);
#define vp8_quantize_mb vp8_quantize_mb_c

void vp8_quantize_mbuv_c(struct macroblock *);
#define vp8_quantize_mbuv vp8_quantize_mbuv_c

void vp8_quantize_mby_c(struct macroblock *);
#define vp8_quantize_mby vp8_quantize_mby_c

int vp8_refining_search_sad_c(struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, int sad_per_bit, int distance, struct variance_vtable *fn_ptr, int *mvcost[2], union int_mv *center_mv);
#define vp8_refining_search_sad vp8_refining_search_sad_c

void vp8_regular_quantize_b_c(struct block *, struct blockd *);
#define vp8_regular_quantize_b vp8_regular_quantize_b_c

void vp8_regular_quantize_b_pair_c(struct block *b1, struct block *b2, struct blockd *d1, struct blockd *d2);
#define vp8_regular_quantize_b_pair vp8_regular_quantize_b_pair_c

unsigned int vp8_sad16x16_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad);
unsigned int vp8_sad16x16_armv6(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad);
#define vp8_sad16x16 vp8_sad16x16_armv6

void vp8_sad16x16x3_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array);
#define vp8_sad16x16x3 vp8_sad16x16x3_c

void vp8_sad16x16x4d_c(const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array);
#define vp8_sad16x16x4d vp8_sad16x16x4d_c

void vp8_sad16x16x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array);
#define vp8_sad16x16x8 vp8_sad16x16x8_c

unsigned int vp8_sad16x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad);
#define vp8_sad16x8 vp8_sad16x8_c

void vp8_sad16x8x3_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array);
#define vp8_sad16x8x3 vp8_sad16x8x3_c

void vp8_sad16x8x4d_c(const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array);
#define vp8_sad16x8x4d vp8_sad16x8x4d_c

void vp8_sad16x8x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array);
#define vp8_sad16x8x8 vp8_sad16x8x8_c

unsigned int vp8_sad4x4_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad);
#define vp8_sad4x4 vp8_sad4x4_c

void vp8_sad4x4x3_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array);
#define vp8_sad4x4x3 vp8_sad4x4x3_c

void vp8_sad4x4x4d_c(const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array);
#define vp8_sad4x4x4d vp8_sad4x4x4d_c

void vp8_sad4x4x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array);
#define vp8_sad4x4x8 vp8_sad4x4x8_c

unsigned int vp8_sad8x16_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad);
#define vp8_sad8x16 vp8_sad8x16_c

void vp8_sad8x16x3_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array);
#define vp8_sad8x16x3 vp8_sad8x16x3_c

void vp8_sad8x16x4d_c(const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array);
#define vp8_sad8x16x4d vp8_sad8x16x4d_c

void vp8_sad8x16x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array);
#define vp8_sad8x16x8 vp8_sad8x16x8_c

unsigned int vp8_sad8x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad);
#define vp8_sad8x8 vp8_sad8x8_c

void vp8_sad8x8x3_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array);
#define vp8_sad8x8x3 vp8_sad8x8x3_c

void vp8_sad8x8x4d_c(const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array);
#define vp8_sad8x8x4d vp8_sad8x8x4d_c

void vp8_sad8x8x8_c(const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array);
#define vp8_sad8x8x8 vp8_sad8x8x8_c

void vp8_short_fdct4x4_c(short *input, short *output, int pitch);
void vp8_short_fdct4x4_armv6(short *input, short *output, int pitch);
#define vp8_short_fdct4x4 vp8_short_fdct4x4_armv6

void vp8_short_fdct8x4_c(short *input, short *output, int pitch);
void vp8_short_fdct8x4_armv6(short *input, short *output, int pitch);
#define vp8_short_fdct8x4 vp8_short_fdct8x4_armv6

void vp8_short_idct4x4llm_c(short *input, unsigned char *pred, int pitch, unsigned char *dst, int dst_stride);
void vp8_short_idct4x4llm_v6_dual(short *input, unsigned char *pred, int pitch, unsigned char *dst, int dst_stride);
#define vp8_short_idct4x4llm vp8_short_idct4x4llm_v6_dual

void vp8_short_inv_walsh4x4_c(short *input, short *output);
void vp8_short_inv_walsh4x4_v6(short *input, short *output);
#define vp8_short_inv_walsh4x4 vp8_short_inv_walsh4x4_v6

void vp8_short_inv_walsh4x4_1_c(short *input, short *output);
#define vp8_short_inv_walsh4x4_1 vp8_short_inv_walsh4x4_1_c

void vp8_short_walsh4x4_c(short *input, short *output, int pitch);
void vp8_short_walsh4x4_armv6(short *input, short *output, int pitch);
#define vp8_short_walsh4x4 vp8_short_walsh4x4_armv6

void vp8_sixtap_predict16x16_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_sixtap_predict16x16_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_sixtap_predict16x16 vp8_sixtap_predict16x16_armv6

void vp8_sixtap_predict4x4_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_sixtap_predict4x4_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_sixtap_predict4x4 vp8_sixtap_predict4x4_armv6

void vp8_sixtap_predict8x4_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_sixtap_predict8x4_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_sixtap_predict8x4 vp8_sixtap_predict8x4_armv6

void vp8_sixtap_predict8x8_c(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
void vp8_sixtap_predict8x8_armv6(unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch);
#define vp8_sixtap_predict8x8 vp8_sixtap_predict8x8_armv6

unsigned int vp8_sub_pixel_mse16x16_c(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
#define vp8_sub_pixel_mse16x16 vp8_sub_pixel_mse16x16_c

unsigned int vp8_sub_pixel_variance16x16_c(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
unsigned int vp8_sub_pixel_variance16x16_armv6(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
#define vp8_sub_pixel_variance16x16 vp8_sub_pixel_variance16x16_armv6

unsigned int vp8_sub_pixel_variance16x8_c(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
#define vp8_sub_pixel_variance16x8 vp8_sub_pixel_variance16x8_c

unsigned int vp8_sub_pixel_variance4x4_c(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
#define vp8_sub_pixel_variance4x4 vp8_sub_pixel_variance4x4_c

unsigned int vp8_sub_pixel_variance8x16_c(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
#define vp8_sub_pixel_variance8x16 vp8_sub_pixel_variance8x16_c

unsigned int vp8_sub_pixel_variance8x8_c(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
unsigned int vp8_sub_pixel_variance8x8_armv6(const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse);
#define vp8_sub_pixel_variance8x8 vp8_sub_pixel_variance8x8_armv6

void vp8_subtract_b_c(struct block *be, struct blockd *bd, int pitch);
void vp8_subtract_b_armv6(struct block *be, struct blockd *bd, int pitch);
#define vp8_subtract_b vp8_subtract_b_armv6

void vp8_subtract_mbuv_c(short *diff, unsigned char *usrc, unsigned char *vsrc, int src_stride, unsigned char *upred, unsigned char *vpred, int pred_stride);
void vp8_subtract_mbuv_armv6(short *diff, unsigned char *usrc, unsigned char *vsrc, int src_stride, unsigned char *upred, unsigned char *vpred, int pred_stride);
#define vp8_subtract_mbuv vp8_subtract_mbuv_armv6

void vp8_subtract_mby_c(short *diff, unsigned char *src, int src_stride, unsigned char *pred, int pred_stride);
void vp8_subtract_mby_armv6(short *diff, unsigned char *src, int src_stride, unsigned char *pred, int pred_stride);
#define vp8_subtract_mby vp8_subtract_mby_armv6

unsigned int vp8_variance16x16_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
unsigned int vp8_variance16x16_armv6(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance16x16 vp8_variance16x16_armv6

unsigned int vp8_variance16x8_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance16x8 vp8_variance16x8_c

unsigned int vp8_variance4x4_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance4x4 vp8_variance4x4_c

unsigned int vp8_variance8x16_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance8x16 vp8_variance8x16_c

unsigned int vp8_variance8x8_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
unsigned int vp8_variance8x8_armv6(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance8x8 vp8_variance8x8_armv6

unsigned int vp8_variance_halfpixvar16x16_h_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
unsigned int vp8_variance_halfpixvar16x16_h_armv6(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance_halfpixvar16x16_h vp8_variance_halfpixvar16x16_h_armv6

unsigned int vp8_variance_halfpixvar16x16_hv_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
unsigned int vp8_variance_halfpixvar16x16_hv_armv6(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance_halfpixvar16x16_hv vp8_variance_halfpixvar16x16_hv_armv6

unsigned int vp8_variance_halfpixvar16x16_v_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
unsigned int vp8_variance_halfpixvar16x16_v_armv6(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse);
#define vp8_variance_halfpixvar16x16_v vp8_variance_halfpixvar16x16_v_armv6

void vp8_yv12_copy_partial_frame_c(struct yv12_buffer_config *src_ybc, struct yv12_buffer_config *dst_ybc);
#define vp8_yv12_copy_partial_frame vp8_yv12_copy_partial_frame_c

void vp8_rtcd(void);

#include "vpx_config.h"

#ifdef RTCD_C
#include "vpx_ports/arm.h"
static void setup_rtcd_internal(void)
{
    int flags = arm_cpu_caps();

    (void)flags;

}
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
