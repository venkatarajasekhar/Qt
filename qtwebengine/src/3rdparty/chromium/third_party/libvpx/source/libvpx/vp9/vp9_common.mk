##
##  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license
##  that can be found in the LICENSE file in the root of the source
##  tree. An additional intellectual property rights grant can be found
##  in the file PATENTS.  All contributing project authors may
##  be found in the AUTHORS file in the root of the source tree.
##

VP9_COMMON_SRCS-yes += vp9_common.mk
VP9_COMMON_SRCS-yes += vp9_iface_common.h
VP9_COMMON_SRCS-yes += common/vp9_pragmas.h
VP9_COMMON_SRCS-yes += common/vp9_ppflags.h
VP9_COMMON_SRCS-yes += common/vp9_alloccommon.c
VP9_COMMON_SRCS-yes += common/vp9_blockd.c
VP9_COMMON_SRCS-yes += common/vp9_convolve.c
VP9_COMMON_SRCS-yes += common/vp9_convolve.h
VP9_COMMON_SRCS-yes += common/vp9_debugmodes.c
VP9_COMMON_SRCS-yes += common/vp9_entropy.c
VP9_COMMON_SRCS-yes += common/vp9_entropymode.c
VP9_COMMON_SRCS-yes += common/vp9_entropymv.c
VP9_COMMON_SRCS-yes += common/vp9_filter.c
VP9_COMMON_SRCS-yes += common/vp9_filter.h
VP9_COMMON_SRCS-yes += common/vp9_frame_buffers.c
VP9_COMMON_SRCS-yes += common/vp9_frame_buffers.h
VP9_COMMON_SRCS-yes += common/vp9_idct.c
VP9_COMMON_SRCS-yes += common/vp9_alloccommon.h
VP9_COMMON_SRCS-yes += common/vp9_blockd.h
VP9_COMMON_SRCS-yes += common/vp9_common.h
VP9_COMMON_SRCS-yes += common/vp9_entropy.h
VP9_COMMON_SRCS-yes += common/vp9_entropymode.h
VP9_COMMON_SRCS-yes += common/vp9_entropymv.h
VP9_COMMON_SRCS-yes += common/vp9_enums.h
VP9_COMMON_SRCS-yes += common/vp9_idct.h
VP9_COMMON_SRCS-yes += common/vp9_loopfilter.h
VP9_COMMON_SRCS-yes += common/vp9_mv.h
VP9_COMMON_SRCS-yes += common/vp9_onyxc_int.h
VP9_COMMON_SRCS-yes += common/vp9_pred_common.h
VP9_COMMON_SRCS-yes += common/vp9_pred_common.c
VP9_COMMON_SRCS-yes += common/vp9_prob.h
VP9_COMMON_SRCS-yes += common/vp9_prob.c
VP9_COMMON_SRCS-yes += common/vp9_quant_common.h
VP9_COMMON_SRCS-yes += common/vp9_reconinter.h
VP9_COMMON_SRCS-yes += common/vp9_reconintra.h
VP9_COMMON_SRCS-yes += common/vp9_rtcd.c
VP9_COMMON_SRCS-yes += common/vp9_rtcd_defs.pl
VP9_COMMON_SRCS-yes += common/vp9_scale.h
VP9_COMMON_SRCS-yes += common/vp9_scale.c
VP9_COMMON_SRCS-yes += common/vp9_seg_common.h
VP9_COMMON_SRCS-yes += common/vp9_seg_common.c
VP9_COMMON_SRCS-yes += common/vp9_systemdependent.h
VP9_COMMON_SRCS-yes += common/vp9_textblit.h
VP9_COMMON_SRCS-yes += common/vp9_tile_common.h
VP9_COMMON_SRCS-yes += common/vp9_tile_common.c
VP9_COMMON_SRCS-yes += common/vp9_loopfilter.c
VP9_COMMON_SRCS-yes += common/vp9_loopfilter_filters.c
VP9_COMMON_SRCS-yes += common/vp9_mvref_common.c
VP9_COMMON_SRCS-yes += common/vp9_mvref_common.h
VP9_COMMON_SRCS-yes += common/vp9_quant_common.c
VP9_COMMON_SRCS-yes += common/vp9_reconinter.c
VP9_COMMON_SRCS-yes += common/vp9_reconintra.c
VP9_COMMON_SRCS-$(CONFIG_POSTPROC_VISUALIZER) += common/vp9_textblit.c
VP9_COMMON_SRCS-yes += common/vp9_common_data.c
VP9_COMMON_SRCS-yes += common/vp9_common_data.h
VP9_COMMON_SRCS-yes += common/vp9_scan.c
VP9_COMMON_SRCS-yes += common/vp9_scan.h

VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_postproc_x86.h
VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_asm_stubs.c
VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_loopfilter_intrin_sse2.c
VP9_COMMON_SRCS-$(HAVE_AVX2) += common/x86/vp9_loopfilter_intrin_avx2.c
VP9_COMMON_SRCS-$(CONFIG_VP9_POSTPROC) += common/vp9_postproc.h
VP9_COMMON_SRCS-$(CONFIG_VP9_POSTPROC) += common/vp9_postproc.c
VP9_COMMON_SRCS-$(HAVE_MMX) += common/x86/vp9_loopfilter_mmx.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_subpixel_8t_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_subpixel_bilinear_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSSE3) += common/x86/vp9_subpixel_8t_ssse3.asm
VP9_COMMON_SRCS-$(HAVE_SSSE3) += common/x86/vp9_subpixel_bilinear_ssse3.asm
VP9_COMMON_SRCS-$(HAVE_AVX2) += common/x86/vp9_subpixel_8t_intrin_avx2.c
VP9_COMMON_SRCS-$(HAVE_SSSE3) += common/x86/vp9_subpixel_8t_intrin_ssse3.c
ifeq ($(CONFIG_VP9_POSTPROC),yes)
VP9_COMMON_SRCS-$(HAVE_MMX) += common/x86/vp9_postproc_mmx.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_postproc_sse2.asm
endif

ifeq ($(CONFIG_USE_X86INC),yes)
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_copy_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_intrapred_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSSE3) += common/x86/vp9_intrapred_ssse3.asm
endif

# common (c)
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_common_dspr2.h
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve2_avg_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve2_avg_horiz_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve2_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve2_horiz_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve2_vert_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve8_avg_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve8_avg_horiz_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve8_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve8_horiz_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_convolve8_vert_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_intrapred4_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_intrapred8_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_intrapred16_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_itrans4_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_itrans8_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_itrans16_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_itrans32_cols_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_itrans32_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_loopfilter_filters_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_loopfilter_filters_dspr2.h
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_loopfilter_macros_dspr2.h
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_loopfilter_masks_dspr2.h
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_mbloop_loopfilter_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_mblpf_horiz_loopfilter_dspr2.c
VP9_COMMON_SRCS-$(HAVE_DSPR2)  += common/mips/dspr2/vp9_mblpf_vert_loopfilter_dspr2.c

VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_idct_intrin_sse2.c

ifeq ($(ARCH_X86_64), yes)
VP9_COMMON_SRCS-$(HAVE_SSSE3) += common/x86/vp9_idct_ssse3.asm
endif

VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_convolve_neon.c
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct16x16_neon.c
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_loopfilter_16_neon.c
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_convolve8_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_convolve8_avg_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_loopfilter_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_loopfilter_16_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_dc_only_idct_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct4x4_1_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct4x4_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct8x8_1_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct8x8_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct16x16_1_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct16x16_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct32x32_1_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_idct32x32_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_iht4x4_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_iht8x8_add_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_mb_lpf_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_copy_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_avg_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_save_reg_neon$(ASM)
VP9_COMMON_SRCS-$(HAVE_NEON_ASM) += common/arm/neon/vp9_reconintra_neon$(ASM)

$(eval $(call rtcd_h_template,vp9_rtcd,vp9/common/vp9_rtcd_defs.pl))
