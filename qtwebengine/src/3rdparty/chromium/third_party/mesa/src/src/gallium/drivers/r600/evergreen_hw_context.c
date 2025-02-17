/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 */
#include "r600_hw_context_priv.h"
#include "evergreend.h"
#include "util/u_memory.h"

static const struct r600_reg evergreen_config_reg_list[] = {
	{R_008958_VGT_PRIMITIVE_TYPE, 0},
};


static const struct r600_reg cayman_config_reg_list[] = {
	{R_008958_VGT_PRIMITIVE_TYPE, 0, 0},
	{R_009100_SPI_CONFIG_CNTL, REG_FLAG_ENABLE_ALWAYS | REG_FLAG_FLUSH_CHANGE, 0},
	{R_00913C_SPI_CONFIG_CNTL_1, REG_FLAG_ENABLE_ALWAYS | REG_FLAG_FLUSH_CHANGE, 0},
};

static const struct r600_reg evergreen_ctl_const_list[] = {
	{R_03CFF4_SQ_VTX_START_INST_LOC, 0, 0},
};

static const struct r600_reg evergreen_context_reg_list[] = {
	{R_028008_DB_DEPTH_VIEW, 0, 0},
	{R_028010_DB_RENDER_OVERRIDE2, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028014_DB_HTILE_DATA_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028040_DB_Z_INFO, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028044_DB_STENCIL_INFO, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028048_DB_Z_READ_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_02804C_DB_STENCIL_READ_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028050_DB_Z_WRITE_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028054_DB_STENCIL_WRITE_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028058_DB_DEPTH_SIZE, 0, 0},
	{R_02805C_DB_DEPTH_SLICE, 0, 0},
	{R_028204_PA_SC_WINDOW_SCISSOR_TL, 0, 0},
	{R_028208_PA_SC_WINDOW_SCISSOR_BR, 0, 0},
	{R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0, 0},
	{R_028250_PA_SC_VPORT_SCISSOR_0_TL, 0, 0},
	{R_028254_PA_SC_VPORT_SCISSOR_0_BR, 0, 0},
	{R_028350_SX_MISC, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028408_VGT_INDX_OFFSET, 0, 0},
	{R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, 0, 0},
	{R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028414_CB_BLEND_RED, 0, 0},
	{R_028418_CB_BLEND_GREEN, 0, 0},
	{R_02841C_CB_BLEND_BLUE, 0, 0},
	{R_028420_CB_BLEND_ALPHA, 0, 0},
	{R_028430_DB_STENCILREFMASK, 0, 0},
	{R_028434_DB_STENCILREFMASK_BF, 0, 0},
	{R_02843C_PA_CL_VPORT_XSCALE_0, 0, 0},
	{R_028440_PA_CL_VPORT_XOFFSET_0, 0, 0},
	{R_028444_PA_CL_VPORT_YSCALE_0, 0, 0},
	{R_028448_PA_CL_VPORT_YOFFSET_0, 0, 0},
	{R_02844C_PA_CL_VPORT_ZSCALE_0, 0, 0},
	{R_028450_PA_CL_VPORT_ZOFFSET_0, 0, 0},
	{R_0285BC_PA_CL_UCP0_X, 0, 0},
	{R_0285C0_PA_CL_UCP0_Y, 0, 0},
	{R_0285C4_PA_CL_UCP0_Z, 0, 0},
	{R_0285C8_PA_CL_UCP0_W, 0, 0},
	{R_0285CC_PA_CL_UCP1_X, 0, 0},
	{R_0285D0_PA_CL_UCP1_Y, 0, 0},
	{R_0285D4_PA_CL_UCP1_Z, 0, 0},
	{R_0285D8_PA_CL_UCP1_W, 0, 0},
	{R_0285DC_PA_CL_UCP2_X, 0, 0},
	{R_0285E0_PA_CL_UCP2_Y, 0, 0},
	{R_0285E4_PA_CL_UCP2_Z, 0, 0},
	{R_0285E8_PA_CL_UCP2_W, 0, 0},
	{R_0285EC_PA_CL_UCP3_X, 0, 0},
	{R_0285F0_PA_CL_UCP3_Y, 0, 0},
	{R_0285F4_PA_CL_UCP3_Z, 0, 0},
	{R_0285F8_PA_CL_UCP3_W, 0, 0},
	{R_0285FC_PA_CL_UCP4_X, 0, 0},
	{R_028600_PA_CL_UCP4_Y, 0, 0},
	{R_028604_PA_CL_UCP4_Z, 0, 0},
	{R_028608_PA_CL_UCP4_W, 0, 0},
	{R_02860C_PA_CL_UCP5_X, 0, 0},
	{R_028610_PA_CL_UCP5_Y, 0, 0},
	{R_028614_PA_CL_UCP5_Z, 0, 0},
	{R_028618_PA_CL_UCP5_W, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_02861C_SPI_VS_OUT_ID_0, 0, 0},
	{R_028620_SPI_VS_OUT_ID_1, 0, 0},
	{R_028624_SPI_VS_OUT_ID_2, 0, 0},
	{R_028628_SPI_VS_OUT_ID_3, 0, 0},
	{R_02862C_SPI_VS_OUT_ID_4, 0, 0},
	{R_028630_SPI_VS_OUT_ID_5, 0, 0},
	{R_028634_SPI_VS_OUT_ID_6, 0, 0},
	{R_028638_SPI_VS_OUT_ID_7, 0, 0},
	{R_02863C_SPI_VS_OUT_ID_8, 0, 0},
	{R_028640_SPI_VS_OUT_ID_9, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028644_SPI_PS_INPUT_CNTL_0, 0, 0},
	{R_028648_SPI_PS_INPUT_CNTL_1, 0, 0},
	{R_02864C_SPI_PS_INPUT_CNTL_2, 0, 0},
	{R_028650_SPI_PS_INPUT_CNTL_3, 0, 0},
	{R_028654_SPI_PS_INPUT_CNTL_4, 0, 0},
	{R_028658_SPI_PS_INPUT_CNTL_5, 0, 0},
	{R_02865C_SPI_PS_INPUT_CNTL_6, 0, 0},
	{R_028660_SPI_PS_INPUT_CNTL_7, 0, 0},
	{R_028664_SPI_PS_INPUT_CNTL_8, 0, 0},
	{R_028668_SPI_PS_INPUT_CNTL_9, 0, 0},
	{R_02866C_SPI_PS_INPUT_CNTL_10, 0, 0},
	{R_028670_SPI_PS_INPUT_CNTL_11, 0, 0},
	{R_028674_SPI_PS_INPUT_CNTL_12, 0, 0},
	{R_028678_SPI_PS_INPUT_CNTL_13, 0, 0},
	{R_02867C_SPI_PS_INPUT_CNTL_14, 0, 0},
	{R_028680_SPI_PS_INPUT_CNTL_15, 0, 0},
	{R_028684_SPI_PS_INPUT_CNTL_16, 0, 0},
	{R_028688_SPI_PS_INPUT_CNTL_17, 0, 0},
	{R_02868C_SPI_PS_INPUT_CNTL_18, 0, 0},
	{R_028690_SPI_PS_INPUT_CNTL_19, 0, 0},
	{R_028694_SPI_PS_INPUT_CNTL_20, 0, 0},
	{R_028698_SPI_PS_INPUT_CNTL_21, 0, 0},
	{R_02869C_SPI_PS_INPUT_CNTL_22, 0, 0},
	{R_0286A0_SPI_PS_INPUT_CNTL_23, 0, 0},
	{R_0286A4_SPI_PS_INPUT_CNTL_24, 0, 0},
	{R_0286A8_SPI_PS_INPUT_CNTL_25, 0, 0},
	{R_0286AC_SPI_PS_INPUT_CNTL_26, 0, 0},
	{R_0286B0_SPI_PS_INPUT_CNTL_27, 0, 0},
	{R_0286B4_SPI_PS_INPUT_CNTL_28, 0, 0},
	{R_0286B8_SPI_PS_INPUT_CNTL_29, 0, 0},
	{R_0286BC_SPI_PS_INPUT_CNTL_30, 0, 0},
	{R_0286C0_SPI_PS_INPUT_CNTL_31, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_0286C4_SPI_VS_OUT_CONFIG, 0, 0},
	{R_0286C8_SPI_THREAD_GROUPING, 0, 0},
	{R_0286CC_SPI_PS_IN_CONTROL_0, 0, 0},
	{R_0286D0_SPI_PS_IN_CONTROL_1, 0, 0},
	{R_0286D4_SPI_INTERP_CONTROL_0, 0, 0},
	{R_0286D8_SPI_INPUT_Z, 0, 0},
	{R_0286E0_SPI_BARYC_CNTL, 0, 0},
	{R_0286E4_SPI_PS_IN_CONTROL_2, 0, 0},
	{R_0286E8_SPI_COMPUTE_INPUT_CNTL, 0, 0},
	{R_028780_CB_BLEND0_CONTROL, 0, 0},
	{R_028784_CB_BLEND1_CONTROL, 0, 0},
	{R_028788_CB_BLEND2_CONTROL, 0, 0},
	{R_02878C_CB_BLEND3_CONTROL, 0, 0},
	{R_028790_CB_BLEND4_CONTROL, 0, 0},
	{R_028794_CB_BLEND5_CONTROL, 0, 0},
	{R_028798_CB_BLEND6_CONTROL, 0, 0},
	{R_02879C_CB_BLEND7_CONTROL, 0, 0},
	{R_028800_DB_DEPTH_CONTROL, 0, 0},
	{R_02880C_DB_SHADER_CONTROL, 0, 0},
	{R_028808_CB_COLOR_CONTROL, 0, 0},
	{R_028810_PA_CL_CLIP_CNTL, 0, 0},
	{R_028814_PA_SU_SC_MODE_CNTL, 0, 0},
	{R_02881C_PA_CL_VS_OUT_CNTL, 0, 0},
	{R_028840_SQ_PGM_START_PS, REG_FLAG_NEED_BO, 0},
	{R_028844_SQ_PGM_RESOURCES_PS, 0, 0},
	{R_02884C_SQ_PGM_EXPORTS_PS, 0, 0},
	{R_02885C_SQ_PGM_START_VS, REG_FLAG_NEED_BO, 0},
	{R_028860_SQ_PGM_RESOURCES_VS, 0, 0},
	{R_0288A4_SQ_PGM_START_FS, REG_FLAG_NEED_BO, 0},
	{R_0288EC_SQ_LDS_ALLOC_PS, 0, 0},
	{R_028A00_PA_SU_POINT_SIZE, 0, 0},
	{R_028A04_PA_SU_POINT_MINMAX, 0, 0},
	{R_028A08_PA_SU_LINE_CNTL, 0, 0},
	{R_028A0C_PA_SC_LINE_STIPPLE, 0, 0},
	{R_028A48_PA_SC_MODE_CNTL_0, 0, 0},
	{R_028A6C_VGT_GS_OUT_PRIM_TYPE, 0, 0},
	{R_028ABC_DB_HTILE_SURFACE, 0, 0},
	{R_028B54_VGT_SHADER_STAGES_EN, 0, 0},
	{R_028B70_DB_ALPHA_TO_MASK, 0, 0},
	{R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL, 0, 0},
	{R_028B7C_PA_SU_POLY_OFFSET_CLAMP, 0, 0},
	{R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE, 0, 0},
	{R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET, 0, 0},
	{R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE, 0, 0},
	{R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET, 0, 0},
	{R_028C00_PA_SC_LINE_CNTL, 0, 0},
	{R_028C04_PA_SC_AA_CONFIG, 0, 0},
	{R_028C08_PA_SU_VTX_CNTL, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028C1C_PA_SC_AA_SAMPLE_LOCS_0, 0, 0},
	{R_028C20_PA_SC_AA_SAMPLE_LOCS_1, 0, 0},
	{R_028C24_PA_SC_AA_SAMPLE_LOCS_2, 0, 0},
	{R_028C28_PA_SC_AA_SAMPLE_LOCS_3, 0, 0},
	{R_028C2C_PA_SC_AA_SAMPLE_LOCS_4, 0, 0},
	{R_028C30_PA_SC_AA_SAMPLE_LOCS_5, 0, 0},
	{R_028C34_PA_SC_AA_SAMPLE_LOCS_6, 0, 0},
	{R_028C38_PA_SC_AA_SAMPLE_LOCS_7, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028C60_CB_COLOR0_BASE, REG_FLAG_NEED_BO, 0},
	{R_028C64_CB_COLOR0_PITCH, 0, 0},
	{R_028C68_CB_COLOR0_SLICE, 0, 0},
	{R_028C6C_CB_COLOR0_VIEW, 0, 0},
	{R_028C70_CB_COLOR0_INFO, REG_FLAG_NEED_BO, 0},
	{R_028C74_CB_COLOR0_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028C78_CB_COLOR0_DIM, 0, 0},
	{R_028C7C_CB_COLOR0_CMASK, REG_FLAG_NEED_BO},
	{R_028C80_CB_COLOR0_CMASK_SLICE},
	{R_028C84_CB_COLOR0_FMASK, REG_FLAG_NEED_BO},
	{R_028C88_CB_COLOR0_FMASK_SLICE},
	{R_028C8C_CB_COLOR0_CLEAR_WORD0},
	{R_028C90_CB_COLOR0_CLEAR_WORD1},
	{R_028C94_CB_COLOR0_CLEAR_WORD2},
	{R_028C98_CB_COLOR0_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028C9C_CB_COLOR1_BASE, REG_FLAG_NEED_BO, 0},
	{R_028CA0_CB_COLOR1_PITCH, 0, 0},
	{R_028CA4_CB_COLOR1_SLICE, 0, 0},
	{R_028CA8_CB_COLOR1_VIEW, 0, 0},
	{R_028CAC_CB_COLOR1_INFO, REG_FLAG_NEED_BO, 0},
	{R_028CB0_CB_COLOR1_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028CB4_CB_COLOR1_DIM, 0, 0},
	{R_028CB8_CB_COLOR1_CMASK, REG_FLAG_NEED_BO, 0},
	{R_028CBC_CB_COLOR1_CMASK_SLICE, 0, 0},
	{R_028CC0_CB_COLOR1_FMASK, REG_FLAG_NEED_BO, 0},
	{R_028CC4_CB_COLOR1_FMASK_SLICE, 0, 0},
	{R_028CC8_CB_COLOR1_CLEAR_WORD0},
	{R_028CCC_CB_COLOR1_CLEAR_WORD1},
	{R_028CD0_CB_COLOR1_CLEAR_WORD2},
	{R_028CD4_CB_COLOR1_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028CD8_CB_COLOR2_BASE, REG_FLAG_NEED_BO, 0},
	{R_028CDC_CB_COLOR2_PITCH, 0, 0},
	{R_028CE0_CB_COLOR2_SLICE, 0, 0},
	{R_028CE4_CB_COLOR2_VIEW, 0, 0},
	{R_028CE8_CB_COLOR2_INFO, REG_FLAG_NEED_BO, 0},
	{R_028CEC_CB_COLOR2_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028CF0_CB_COLOR2_DIM, 0, 0},
	{R_028CF4_CB_COLOR2_CMASK, REG_FLAG_NEED_BO, 0},
	{R_028CF8_CB_COLOR2_CMASK_SLICE, 0, 0},
	{R_028CFC_CB_COLOR2_FMASK, REG_FLAG_NEED_BO, 0},
	{R_028D00_CB_COLOR2_FMASK_SLICE, 0, 0},
	{R_028D04_CB_COLOR2_CLEAR_WORD0},
	{R_028D08_CB_COLOR2_CLEAR_WORD1},
	{R_028D0C_CB_COLOR2_CLEAR_WORD2},
	{R_028D10_CB_COLOR2_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028D14_CB_COLOR3_BASE, REG_FLAG_NEED_BO, 0},
	{R_028D18_CB_COLOR3_PITCH, 0, 0},
	{R_028D1C_CB_COLOR3_SLICE, 0, 0},
	{R_028D20_CB_COLOR3_VIEW, 0, 0},
	{R_028D24_CB_COLOR3_INFO, REG_FLAG_NEED_BO, 0},
	{R_028D28_CB_COLOR3_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028D2C_CB_COLOR3_DIM, 0, 0},
	{R_028D30_CB_COLOR3_CMASK, REG_FLAG_NEED_BO},
	{R_028D34_CB_COLOR3_CMASK_SLICE},
	{R_028D38_CB_COLOR3_FMASK, REG_FLAG_NEED_BO},
	{R_028D3C_CB_COLOR3_FMASK_SLICE},
	{R_028D40_CB_COLOR3_CLEAR_WORD0},
	{R_028D44_CB_COLOR3_CLEAR_WORD1},
	{R_028D48_CB_COLOR3_CLEAR_WORD2},
	{R_028D4C_CB_COLOR3_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028D50_CB_COLOR4_BASE, REG_FLAG_NEED_BO, 0},
	{R_028D54_CB_COLOR4_PITCH, 0, 0},
	{R_028D58_CB_COLOR4_SLICE, 0, 0},
	{R_028D5C_CB_COLOR4_VIEW, 0, 0},
	{R_028D60_CB_COLOR4_INFO, REG_FLAG_NEED_BO, 0},
	{R_028D64_CB_COLOR4_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028D68_CB_COLOR4_DIM, 0, 0},
	{R_028D6C_CB_COLOR4_CMASK, REG_FLAG_NEED_BO},
	{R_028D70_CB_COLOR4_CMASK_SLICE},
	{R_028D74_CB_COLOR4_FMASK, REG_FLAG_NEED_BO},
	{R_028D78_CB_COLOR4_FMASK_SLICE},
	{R_028D7C_CB_COLOR4_CLEAR_WORD0},
	{R_028D80_CB_COLOR4_CLEAR_WORD1},
	{R_028D84_CB_COLOR4_CLEAR_WORD2},
	{R_028D88_CB_COLOR4_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028D8C_CB_COLOR5_BASE, REG_FLAG_NEED_BO, 0},
	{R_028D90_CB_COLOR5_PITCH, 0, 0},
	{R_028D94_CB_COLOR5_SLICE, 0, 0},
	{R_028D98_CB_COLOR5_VIEW, 0, 0},
	{R_028D9C_CB_COLOR5_INFO, REG_FLAG_NEED_BO, 0},
	{R_028DA0_CB_COLOR5_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028DA4_CB_COLOR5_DIM, 0, 0},
	{R_028DA8_CB_COLOR5_CMASK, REG_FLAG_NEED_BO},
	{R_028DAC_CB_COLOR5_CMASK_SLICE},
	{R_028DB0_CB_COLOR5_FMASK, REG_FLAG_NEED_BO},
	{R_028DB4_CB_COLOR5_FMASK_SLICE},
	{R_028DB8_CB_COLOR5_CLEAR_WORD0},
	{R_028DBC_CB_COLOR5_CLEAR_WORD1},
	{R_028DC0_CB_COLOR5_CLEAR_WORD2},
	{R_028DC4_CB_COLOR5_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028DC8_CB_COLOR6_BASE, REG_FLAG_NEED_BO, 0},
	{R_028DCC_CB_COLOR6_PITCH, 0, 0},
	{R_028DD0_CB_COLOR6_SLICE, 0, 0},
	{R_028DD4_CB_COLOR6_VIEW, 0, 0},
	{R_028DD8_CB_COLOR6_INFO, REG_FLAG_NEED_BO, 0},
	{R_028DDC_CB_COLOR6_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028DE0_CB_COLOR6_DIM, 0, 0},
	{R_028DE4_CB_COLOR6_CMASK, REG_FLAG_NEED_BO},
	{R_028DE8_CB_COLOR6_CMASK_SLICE},
	{R_028DEC_CB_COLOR6_FMASK, REG_FLAG_NEED_BO},
	{R_028DF0_CB_COLOR6_FMASK_SLICE},
	{R_028DF4_CB_COLOR6_CLEAR_WORD0},
	{R_028DF8_CB_COLOR6_CLEAR_WORD1},
	{R_028DFC_CB_COLOR6_CLEAR_WORD2},
	{R_028E00_CB_COLOR6_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E04_CB_COLOR7_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E08_CB_COLOR7_PITCH, 0, 0},
	{R_028E0C_CB_COLOR7_SLICE, 0, 0},
	{R_028E10_CB_COLOR7_VIEW, 0, 0},
	{R_028E14_CB_COLOR7_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E18_CB_COLOR7_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E1C_CB_COLOR7_DIM, 0, 0},
	{R_028E20_CB_COLOR7_CMASK, REG_FLAG_NEED_BO},
	{R_028E24_CB_COLOR7_CMASK_SLICE},
	{R_028E28_CB_COLOR7_FMASK, REG_FLAG_NEED_BO},
	{R_028E2C_CB_COLOR7_FMASK_SLICE},
	{R_028E30_CB_COLOR7_CLEAR_WORD0},
	{R_028E34_CB_COLOR7_CLEAR_WORD1},
	{R_028E38_CB_COLOR7_CLEAR_WORD2},
	{R_028E3C_CB_COLOR7_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E40_CB_COLOR8_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E44_CB_COLOR8_PITCH, 0, 0},
	{R_028E48_CB_COLOR8_SLICE, 0, 0},
	{R_028E4C_CB_COLOR8_VIEW, 0, 0},
	{R_028E50_CB_COLOR8_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E54_CB_COLOR8_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E58_CB_COLOR8_DIM, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E5C_CB_COLOR9_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E60_CB_COLOR9_PITCH, 0, 0},
	{R_028E64_CB_COLOR9_SLICE, 0, 0},
	{R_028E68_CB_COLOR9_VIEW, 0, 0},
	{R_028E6C_CB_COLOR9_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E70_CB_COLOR9_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E74_CB_COLOR9_DIM, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E78_CB_COLOR10_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E7C_CB_COLOR10_PITCH, 0, 0},
	{R_028E80_CB_COLOR10_SLICE, 0, 0},
	{R_028E84_CB_COLOR10_VIEW, 0, 0},
	{R_028E88_CB_COLOR10_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E8C_CB_COLOR10_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E90_CB_COLOR10_DIM, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E94_CB_COLOR11_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E98_CB_COLOR11_PITCH, 0, 0},
	{R_028E9C_CB_COLOR11_SLICE, 0, 0},
	{R_028EA0_CB_COLOR11_VIEW, 0, 0},
	{R_028EA4_CB_COLOR11_INFO, REG_FLAG_NEED_BO, 0},
	{R_028EA8_CB_COLOR11_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028EAC_CB_COLOR11_DIM, 0, 0},
};

static const struct r600_reg cayman_context_reg_list[] = {
	{R_028008_DB_DEPTH_VIEW, 0, 0},
	{R_028010_DB_RENDER_OVERRIDE2, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028014_DB_HTILE_DATA_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028040_DB_Z_INFO, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028044_DB_STENCIL_INFO, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028048_DB_Z_READ_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_02804C_DB_STENCIL_READ_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028050_DB_Z_WRITE_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028054_DB_STENCIL_WRITE_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028058_DB_DEPTH_SIZE, 0, 0},
	{R_02805C_DB_DEPTH_SLICE, 0, 0},
	{R_028204_PA_SC_WINDOW_SCISSOR_TL, 0, 0},
	{R_028208_PA_SC_WINDOW_SCISSOR_BR, 0, 0},
	{R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0, 0},
	{R_028250_PA_SC_VPORT_SCISSOR_0_TL, 0, 0},
	{R_028254_PA_SC_VPORT_SCISSOR_0_BR, 0, 0},
	{R_028350_SX_MISC, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028408_VGT_INDX_OFFSET, 0, 0},
	{R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, 0, 0},
	{R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028414_CB_BLEND_RED, 0, 0},
	{R_028418_CB_BLEND_GREEN, 0, 0},
	{R_02841C_CB_BLEND_BLUE, 0, 0},
	{R_028420_CB_BLEND_ALPHA, 0, 0},
	{R_028430_DB_STENCILREFMASK, 0, 0},
	{R_028434_DB_STENCILREFMASK_BF, 0, 0},
	{R_02843C_PA_CL_VPORT_XSCALE_0, 0, 0},
	{R_028440_PA_CL_VPORT_XOFFSET_0, 0, 0},
	{R_028444_PA_CL_VPORT_YSCALE_0, 0, 0},
	{R_028448_PA_CL_VPORT_YOFFSET_0, 0, 0},
	{R_02844C_PA_CL_VPORT_ZSCALE_0, 0, 0},
	{R_028450_PA_CL_VPORT_ZOFFSET_0, 0, 0},
	{R_0285BC_PA_CL_UCP0_X, 0, 0},
	{R_0285C0_PA_CL_UCP0_Y, 0, 0},
	{R_0285C4_PA_CL_UCP0_Z, 0, 0},
	{R_0285C8_PA_CL_UCP0_W, 0, 0},
	{R_0285CC_PA_CL_UCP1_X, 0, 0},
	{R_0285D0_PA_CL_UCP1_Y, 0, 0},
	{R_0285D4_PA_CL_UCP1_Z, 0, 0},
	{R_0285D8_PA_CL_UCP1_W, 0, 0},
	{R_0285DC_PA_CL_UCP2_X, 0, 0},
	{R_0285E0_PA_CL_UCP2_Y, 0, 0},
	{R_0285E4_PA_CL_UCP2_Z, 0, 0},
	{R_0285E8_PA_CL_UCP2_W, 0, 0},
	{R_0285EC_PA_CL_UCP3_X, 0, 0},
	{R_0285F0_PA_CL_UCP3_Y, 0, 0},
	{R_0285F4_PA_CL_UCP3_Z, 0, 0},
	{R_0285F8_PA_CL_UCP3_W, 0, 0},
	{R_0285FC_PA_CL_UCP4_X, 0, 0},
	{R_028600_PA_CL_UCP4_Y, 0, 0},
	{R_028604_PA_CL_UCP4_Z, 0, 0},
	{R_028608_PA_CL_UCP4_W, 0, 0},
	{R_02860C_PA_CL_UCP5_X, 0, 0},
	{R_028610_PA_CL_UCP5_Y, 0, 0},
	{R_028614_PA_CL_UCP5_Z, 0, 0},
	{R_028618_PA_CL_UCP5_W, 0, 0},
	{R_02861C_SPI_VS_OUT_ID_0, 0, 0},
	{R_028620_SPI_VS_OUT_ID_1, 0, 0},
	{R_028624_SPI_VS_OUT_ID_2, 0, 0},
	{R_028628_SPI_VS_OUT_ID_3, 0, 0},
	{R_02862C_SPI_VS_OUT_ID_4, 0, 0},
	{R_028630_SPI_VS_OUT_ID_5, 0, 0},
	{R_028634_SPI_VS_OUT_ID_6, 0, 0},
	{R_028638_SPI_VS_OUT_ID_7, 0, 0},
	{R_02863C_SPI_VS_OUT_ID_8, 0, 0},
	{R_028640_SPI_VS_OUT_ID_9, 0, 0},
	{R_028644_SPI_PS_INPUT_CNTL_0, 0, 0},
	{R_028648_SPI_PS_INPUT_CNTL_1, 0, 0},
	{R_02864C_SPI_PS_INPUT_CNTL_2, 0, 0},
	{R_028650_SPI_PS_INPUT_CNTL_3, 0, 0},
	{R_028654_SPI_PS_INPUT_CNTL_4, 0, 0},
	{R_028658_SPI_PS_INPUT_CNTL_5, 0, 0},
	{R_02865C_SPI_PS_INPUT_CNTL_6, 0, 0},
	{R_028660_SPI_PS_INPUT_CNTL_7, 0, 0},
	{R_028664_SPI_PS_INPUT_CNTL_8, 0, 0},
	{R_028668_SPI_PS_INPUT_CNTL_9, 0, 0},
	{R_02866C_SPI_PS_INPUT_CNTL_10, 0, 0},
	{R_028670_SPI_PS_INPUT_CNTL_11, 0, 0},
	{R_028674_SPI_PS_INPUT_CNTL_12, 0, 0},
	{R_028678_SPI_PS_INPUT_CNTL_13, 0, 0},
	{R_02867C_SPI_PS_INPUT_CNTL_14, 0, 0},
	{R_028680_SPI_PS_INPUT_CNTL_15, 0, 0},
	{R_028684_SPI_PS_INPUT_CNTL_16, 0, 0},
	{R_028688_SPI_PS_INPUT_CNTL_17, 0, 0},
	{R_02868C_SPI_PS_INPUT_CNTL_18, 0, 0},
	{R_028690_SPI_PS_INPUT_CNTL_19, 0, 0},
	{R_028694_SPI_PS_INPUT_CNTL_20, 0, 0},
	{R_028698_SPI_PS_INPUT_CNTL_21, 0, 0},
	{R_02869C_SPI_PS_INPUT_CNTL_22, 0, 0},
	{R_0286A0_SPI_PS_INPUT_CNTL_23, 0, 0},
	{R_0286A4_SPI_PS_INPUT_CNTL_24, 0, 0},
	{R_0286A8_SPI_PS_INPUT_CNTL_25, 0, 0},
	{R_0286AC_SPI_PS_INPUT_CNTL_26, 0, 0},
	{R_0286B0_SPI_PS_INPUT_CNTL_27, 0, 0},
	{R_0286B4_SPI_PS_INPUT_CNTL_28, 0, 0},
	{R_0286B8_SPI_PS_INPUT_CNTL_29, 0, 0},
	{R_0286BC_SPI_PS_INPUT_CNTL_30, 0, 0},
	{R_0286C0_SPI_PS_INPUT_CNTL_31, 0, 0},
	{R_0286C4_SPI_VS_OUT_CONFIG, 0, 0},
	{R_0286C8_SPI_THREAD_GROUPING, 0, 0},
	{R_0286CC_SPI_PS_IN_CONTROL_0, 0, 0},
	{R_0286D0_SPI_PS_IN_CONTROL_1, 0, 0},
	{R_0286D4_SPI_INTERP_CONTROL_0, 0, 0},
	{R_0286D8_SPI_INPUT_Z, 0, 0},
	{R_0286E0_SPI_BARYC_CNTL, 0, 0},
	{R_0286E4_SPI_PS_IN_CONTROL_2, 0, 0},
	{R_0286E8_SPI_COMPUTE_INPUT_CNTL, 0, 0},
	{R_028780_CB_BLEND0_CONTROL, 0, 0},
	{R_028784_CB_BLEND1_CONTROL, 0, 0},
	{R_028788_CB_BLEND2_CONTROL, 0, 0},
	{R_02878C_CB_BLEND3_CONTROL, 0, 0},
	{R_028790_CB_BLEND4_CONTROL, 0, 0},
	{R_028794_CB_BLEND5_CONTROL, 0, 0},
	{R_028798_CB_BLEND6_CONTROL, 0, 0},
	{R_02879C_CB_BLEND7_CONTROL, 0, 0},
	{R_028800_DB_DEPTH_CONTROL, 0, 0},
	{CM_R_028804_DB_EQAA},
	{R_028808_CB_COLOR_CONTROL, 0, 0},
	{R_02880C_DB_SHADER_CONTROL, 0, 0},
	{R_028810_PA_CL_CLIP_CNTL, 0, 0},
	{R_028814_PA_SU_SC_MODE_CNTL, 0, 0},
	{R_02881C_PA_CL_VS_OUT_CNTL, 0, 0},
	{R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1, 0, 0},
	{R_028840_SQ_PGM_START_PS, REG_FLAG_NEED_BO, 0},
	{R_028844_SQ_PGM_RESOURCES_PS, 0, 0},
	{R_02884C_SQ_PGM_EXPORTS_PS, 0, 0},
	{R_02885C_SQ_PGM_START_VS, REG_FLAG_NEED_BO, 0},
	{R_028860_SQ_PGM_RESOURCES_VS, 0, 0},
	{R_0288A4_SQ_PGM_START_FS, REG_FLAG_NEED_BO, 0},
	{R_028900_SQ_ESGS_RING_ITEMSIZE, 0, 0},
	{R_028904_SQ_GSVS_RING_ITEMSIZE, 0, 0},
	{R_028908_SQ_ESTMP_RING_ITEMSIZE, 0, 0},
	{R_02890C_SQ_GSTMP_RING_ITEMSIZE, 0, 0},
	{R_028910_SQ_VSTMP_RING_ITEMSIZE, 0, 0},
	{R_028914_SQ_PSTMP_RING_ITEMSIZE, 0, 0},
	{R_02891C_SQ_GS_VERT_ITEMSIZE, 0, 0},
	{R_028920_SQ_GS_VERT_ITEMSIZE_1, 0, 0},
	{R_028924_SQ_GS_VERT_ITEMSIZE_2, 0, 0},
	{R_028928_SQ_GS_VERT_ITEMSIZE_3, 0, 0},
	{R_028A00_PA_SU_POINT_SIZE, 0, 0},
	{R_028A04_PA_SU_POINT_MINMAX, 0, 0},
	{R_028A08_PA_SU_LINE_CNTL, 0, 0},
	{R_028A0C_PA_SC_LINE_STIPPLE, 0, 0},
	{R_028A48_PA_SC_MODE_CNTL_0, 0, 0},
	{R_028A6C_VGT_GS_OUT_PRIM_TYPE, 0, 0},
	{R_028ABC_DB_HTILE_SURFACE, 0, 0},
	{R_028B54_VGT_SHADER_STAGES_EN, 0, 0},
	{R_028B70_DB_ALPHA_TO_MASK, 0, 0},
	{R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL, 0, 0},
	{R_028B7C_PA_SU_POLY_OFFSET_CLAMP, 0, 0},
	{R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE, 0, 0},
	{R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET, 0, 0},
	{R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE, 0, 0},
	{R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET, 0, 0},
	{CM_R_028BDC_PA_SC_LINE_CNTL, 0, 0},
	{CM_R_028BE0_PA_SC_AA_CONFIG, 0, 0},
	{CM_R_028BE4_PA_SU_VTX_CNTL, 0, 0},
	{CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, 0, 0},
	{CM_R_028BFC_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_1, 0, 0},
	{CM_R_028C00_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_2, 0, 0},
	{CM_R_028C04_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_3, 0, 0},
	{CM_R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0, 0, 0},
	{CM_R_028C0C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_1, 0, 0},
	{CM_R_028C10_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_2, 0, 0},
	{CM_R_028C14_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_3, 0, 0},
	{CM_R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0, 0, 0},
	{CM_R_028C1C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_1, 0, 0},
	{CM_R_028C20_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_2, 0, 0},
	{CM_R_028C24_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_3, 0, 0},
	{CM_R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0, 0, 0},
	{CM_R_028C2C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_1, 0, 0},
	{CM_R_028C30_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_2, 0, 0},
	{CM_R_028C34_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_3, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028C60_CB_COLOR0_BASE, REG_FLAG_NEED_BO, 0},
	{R_028C64_CB_COLOR0_PITCH, 0, 0},
	{R_028C68_CB_COLOR0_SLICE, 0, 0},
	{R_028C6C_CB_COLOR0_VIEW, 0, 0},
	{R_028C70_CB_COLOR0_INFO, REG_FLAG_NEED_BO, 0},
	{R_028C74_CB_COLOR0_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028C78_CB_COLOR0_DIM, 0, 0},
	{R_028C7C_CB_COLOR0_CMASK, REG_FLAG_NEED_BO},
	{R_028C80_CB_COLOR0_CMASK_SLICE},
	{R_028C84_CB_COLOR0_FMASK, REG_FLAG_NEED_BO},
	{R_028C88_CB_COLOR0_FMASK_SLICE},
	{R_028C8C_CB_COLOR0_CLEAR_WORD0},
	{R_028C90_CB_COLOR0_CLEAR_WORD1},
	{R_028C94_CB_COLOR0_CLEAR_WORD2},
	{R_028C98_CB_COLOR0_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028C9C_CB_COLOR1_BASE, REG_FLAG_NEED_BO, 0},
	{R_028CA0_CB_COLOR1_PITCH, 0, 0},
	{R_028CA4_CB_COLOR1_SLICE, 0, 0},
	{R_028CA8_CB_COLOR1_VIEW, 0, 0},
	{R_028CAC_CB_COLOR1_INFO, REG_FLAG_NEED_BO, 0},
	{R_028CB0_CB_COLOR1_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028CB4_CB_COLOR1_DIM, 0, 0},
	{R_028CB8_CB_COLOR1_CMASK, REG_FLAG_NEED_BO, 0},
	{R_028CBC_CB_COLOR1_CMASK_SLICE, 0, 0},
	{R_028CC0_CB_COLOR1_FMASK, REG_FLAG_NEED_BO, 0},
	{R_028CC4_CB_COLOR1_FMASK_SLICE, 0, 0},
	{R_028CC8_CB_COLOR1_CLEAR_WORD0},
	{R_028CCC_CB_COLOR1_CLEAR_WORD1},
	{R_028CD0_CB_COLOR1_CLEAR_WORD2},
	{R_028CD4_CB_COLOR1_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028CD8_CB_COLOR2_BASE, REG_FLAG_NEED_BO, 0},
	{R_028CDC_CB_COLOR2_PITCH, 0, 0},
	{R_028CE0_CB_COLOR2_SLICE, 0, 0},
	{R_028CE4_CB_COLOR2_VIEW, 0, 0},
	{R_028CE8_CB_COLOR2_INFO, REG_FLAG_NEED_BO, 0},
	{R_028CEC_CB_COLOR2_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028CF0_CB_COLOR2_DIM, 0, 0},
	{R_028CF4_CB_COLOR2_CMASK, REG_FLAG_NEED_BO, 0},
	{R_028CF8_CB_COLOR2_CMASK_SLICE, 0, 0},
	{R_028CFC_CB_COLOR2_FMASK, REG_FLAG_NEED_BO, 0},
	{R_028D00_CB_COLOR2_FMASK_SLICE, 0, 0},
	{R_028D04_CB_COLOR2_CLEAR_WORD0},
	{R_028D08_CB_COLOR2_CLEAR_WORD1},
	{R_028D0C_CB_COLOR2_CLEAR_WORD2},
	{R_028D10_CB_COLOR2_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028D14_CB_COLOR3_BASE, REG_FLAG_NEED_BO, 0},
	{R_028D18_CB_COLOR3_PITCH, 0, 0},
	{R_028D1C_CB_COLOR3_SLICE, 0, 0},
	{R_028D20_CB_COLOR3_VIEW, 0, 0},
	{R_028D24_CB_COLOR3_INFO, REG_FLAG_NEED_BO, 0},
	{R_028D28_CB_COLOR3_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028D2C_CB_COLOR3_DIM, 0, 0},
	{R_028D30_CB_COLOR3_CMASK, REG_FLAG_NEED_BO},
	{R_028D34_CB_COLOR3_CMASK_SLICE},
	{R_028D38_CB_COLOR3_FMASK, REG_FLAG_NEED_BO},
	{R_028D3C_CB_COLOR3_FMASK_SLICE},
	{R_028D40_CB_COLOR3_CLEAR_WORD0},
	{R_028D44_CB_COLOR3_CLEAR_WORD1},
	{R_028D48_CB_COLOR3_CLEAR_WORD2},
	{R_028D4C_CB_COLOR3_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028D50_CB_COLOR4_BASE, REG_FLAG_NEED_BO, 0},
	{R_028D54_CB_COLOR4_PITCH, 0, 0},
	{R_028D58_CB_COLOR4_SLICE, 0, 0},
	{R_028D5C_CB_COLOR4_VIEW, 0, 0},
	{R_028D60_CB_COLOR4_INFO, REG_FLAG_NEED_BO, 0},
	{R_028D64_CB_COLOR4_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028D68_CB_COLOR4_DIM, 0, 0},
	{R_028D6C_CB_COLOR4_CMASK, REG_FLAG_NEED_BO},
	{R_028D70_CB_COLOR4_CMASK_SLICE},
	{R_028D74_CB_COLOR4_FMASK, REG_FLAG_NEED_BO},
	{R_028D78_CB_COLOR4_FMASK_SLICE},
	{R_028D7C_CB_COLOR4_CLEAR_WORD0},
	{R_028D80_CB_COLOR4_CLEAR_WORD1},
	{R_028D84_CB_COLOR4_CLEAR_WORD2},
	{R_028D88_CB_COLOR4_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028D8C_CB_COLOR5_BASE, REG_FLAG_NEED_BO, 0},
	{R_028D90_CB_COLOR5_PITCH, 0, 0},
	{R_028D94_CB_COLOR5_SLICE, 0, 0},
	{R_028D98_CB_COLOR5_VIEW, 0, 0},
	{R_028D9C_CB_COLOR5_INFO, REG_FLAG_NEED_BO, 0},
	{R_028DA0_CB_COLOR5_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028DA4_CB_COLOR5_DIM, 0, 0},
	{R_028DA8_CB_COLOR5_CMASK, REG_FLAG_NEED_BO},
	{R_028DAC_CB_COLOR5_CMASK_SLICE},
	{R_028DB0_CB_COLOR5_FMASK, REG_FLAG_NEED_BO},
	{R_028DB4_CB_COLOR5_FMASK_SLICE},
	{R_028DB8_CB_COLOR5_CLEAR_WORD0},
	{R_028DBC_CB_COLOR5_CLEAR_WORD1},
	{R_028DC0_CB_COLOR5_CLEAR_WORD2},
	{R_028DC4_CB_COLOR5_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028DC8_CB_COLOR6_BASE, REG_FLAG_NEED_BO, 0},
	{R_028DCC_CB_COLOR6_PITCH, 0, 0},
	{R_028DD0_CB_COLOR6_SLICE, 0, 0},
	{R_028DD4_CB_COLOR6_VIEW, 0, 0},
	{R_028DD8_CB_COLOR6_INFO, REG_FLAG_NEED_BO, 0},
	{R_028DDC_CB_COLOR6_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028DE0_CB_COLOR6_DIM, 0, 0},
	{R_028DE4_CB_COLOR6_CMASK, REG_FLAG_NEED_BO},
	{R_028DE8_CB_COLOR6_CMASK_SLICE},
	{R_028DEC_CB_COLOR6_FMASK, REG_FLAG_NEED_BO},
	{R_028DF0_CB_COLOR6_FMASK_SLICE},
	{R_028DF4_CB_COLOR6_CLEAR_WORD0},
	{R_028DF8_CB_COLOR6_CLEAR_WORD1},
	{R_028DFC_CB_COLOR6_CLEAR_WORD2},
	{R_028E00_CB_COLOR6_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E04_CB_COLOR7_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E08_CB_COLOR7_PITCH, 0, 0},
	{R_028E0C_CB_COLOR7_SLICE, 0, 0},
	{R_028E10_CB_COLOR7_VIEW, 0, 0},
	{R_028E14_CB_COLOR7_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E18_CB_COLOR7_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E1C_CB_COLOR7_DIM, 0, 0},
	{R_028E20_CB_COLOR7_CMASK, REG_FLAG_NEED_BO},
	{R_028E24_CB_COLOR7_CMASK_SLICE},
	{R_028E28_CB_COLOR7_FMASK, REG_FLAG_NEED_BO},
	{R_028E2C_CB_COLOR7_FMASK_SLICE},
	{R_028E30_CB_COLOR7_CLEAR_WORD0},
	{R_028E34_CB_COLOR7_CLEAR_WORD1},
	{R_028E38_CB_COLOR7_CLEAR_WORD2},
	{R_028E3C_CB_COLOR7_CLEAR_WORD3},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E40_CB_COLOR8_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E44_CB_COLOR8_PITCH, 0, 0},
	{R_028E48_CB_COLOR8_SLICE, 0, 0},
	{R_028E4C_CB_COLOR8_VIEW, 0, 0},
	{R_028E50_CB_COLOR8_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E54_CB_COLOR8_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E58_CB_COLOR8_DIM, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E5C_CB_COLOR9_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E60_CB_COLOR9_PITCH, 0, 0},
	{R_028E64_CB_COLOR9_SLICE, 0, 0},
	{R_028E68_CB_COLOR9_VIEW, 0, 0},
	{R_028E6C_CB_COLOR9_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E70_CB_COLOR9_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E74_CB_COLOR9_DIM, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E78_CB_COLOR10_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E7C_CB_COLOR10_PITCH, 0, 0},
	{R_028E80_CB_COLOR10_SLICE, 0, 0},
	{R_028E84_CB_COLOR10_VIEW, 0, 0},
	{R_028E88_CB_COLOR10_INFO, REG_FLAG_NEED_BO, 0},
	{R_028E8C_CB_COLOR10_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028E90_CB_COLOR10_DIM, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028E94_CB_COLOR11_BASE, REG_FLAG_NEED_BO, 0},
	{R_028E98_CB_COLOR11_PITCH, 0, 0},
	{R_028E9C_CB_COLOR11_SLICE, 0, 0},
	{R_028EA0_CB_COLOR11_VIEW, 0, 0},
	{R_028EA4_CB_COLOR11_INFO, REG_FLAG_NEED_BO, 0},
	{R_028EA8_CB_COLOR11_ATTRIB, REG_FLAG_NEED_BO, 0},
	{R_028EAC_CB_COLOR11_DIM, 0, 0},
};

static int evergreen_loop_const_init(struct r600_context *ctx, uint32_t offset)
{
	unsigned nreg = 32;
	struct r600_reg r600_loop_consts[32];
	int i;

	for (i = 0; i < nreg; i++) {
		r600_loop_consts[i].offset = EVERGREEN_LOOP_CONST_OFFSET + ((offset + i) * 4);
		r600_loop_consts[i].flags = REG_FLAG_DIRTY_ALWAYS;
		r600_loop_consts[i].sbu_flags = 0;
	}
	return r600_context_add_block(ctx, r600_loop_consts, nreg, PKT3_SET_LOOP_CONST, EVERGREEN_LOOP_CONST_OFFSET);
}

int evergreen_context_init(struct r600_context *ctx)
{
	int r;

	/* add blocks */
	if (ctx->family >= CHIP_CAYMAN)
		r = r600_context_add_block(ctx, cayman_config_reg_list,
					   Elements(cayman_config_reg_list), PKT3_SET_CONFIG_REG, EVERGREEN_CONFIG_REG_OFFSET);
	else
		r = r600_context_add_block(ctx, evergreen_config_reg_list,
					   Elements(evergreen_config_reg_list), PKT3_SET_CONFIG_REG, EVERGREEN_CONFIG_REG_OFFSET);
	if (r)
		goto out_err;
	if (ctx->family >= CHIP_CAYMAN)
		r = r600_context_add_block(ctx, cayman_context_reg_list,
					   Elements(cayman_context_reg_list), PKT3_SET_CONTEXT_REG, EVERGREEN_CONTEXT_REG_OFFSET);
	else
		r = r600_context_add_block(ctx, evergreen_context_reg_list,
					   Elements(evergreen_context_reg_list), PKT3_SET_CONTEXT_REG, EVERGREEN_CONTEXT_REG_OFFSET);
	if (r)
		goto out_err;
	r = r600_context_add_block(ctx, evergreen_ctl_const_list,
				   Elements(evergreen_ctl_const_list), PKT3_SET_CTL_CONST, EVERGREEN_CTL_CONST_OFFSET);
	if (r)
		goto out_err;

	/* PS loop const */
	evergreen_loop_const_init(ctx, 0);
	/* VS loop const */
	evergreen_loop_const_init(ctx, 32);

	r = r600_setup_block_table(ctx);
	if (r)
		goto out_err;

	ctx->max_db = 8;
	return 0;
out_err:
	r600_context_fini(ctx);
	return r;
}

void evergreen_flush_vgt_streamout(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONFIG_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_0084FC_CP_STRMOUT_CNTL - EVERGREEN_CONFIG_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = 0;

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0);

	cs->buf[cs->cdw++] = PKT3(PKT3_WAIT_REG_MEM, 5, 0);
	cs->buf[cs->cdw++] = WAIT_REG_MEM_EQUAL; /* wait until the register is equal to the reference value */
	cs->buf[cs->cdw++] = R_0084FC_CP_STRMOUT_CNTL >> 2;  /* register */
	cs->buf[cs->cdw++] = 0;
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* reference value */
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* mask */
	cs->buf[cs->cdw++] = 4; /* poll interval */
}

void evergreen_set_streamout_enable(struct r600_context *ctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	if (buffer_enable_bit) {
		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B94_VGT_STRMOUT_CONFIG - EVERGREEN_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B94_STREAMOUT_0_EN(1);

		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B98_VGT_STRMOUT_BUFFER_CONFIG - EVERGREEN_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B98_STREAM_0_BUFFER_EN(buffer_enable_bit);
	} else {
		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B94_VGT_STRMOUT_CONFIG - EVERGREEN_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B94_STREAMOUT_0_EN(0);
	}
}
