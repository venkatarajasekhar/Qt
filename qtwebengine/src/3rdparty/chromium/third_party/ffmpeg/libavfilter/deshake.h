/*
 * Copyright (C) 2013 Wei Gao <weigao@multicorewareinc.com>
 * Copyright (C) 2013 Lenny Wang
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFILTER_DESHAKE_H
#define AVFILTER_DESHAKE_H

#include "config.h"
#include "avfilter.h"
#include "libavcodec/dsputil.h"
#include "transform.h"
#if CONFIG_OPENCL
#include "libavutil/opencl.h"
#endif


enum SearchMethod {
    EXHAUSTIVE,        ///< Search all possible positions
    SMART_EXHAUSTIVE,  ///< Search most possible positions (faster)
    SEARCH_COUNT
};

typedef struct {
    int x;             ///< Horizontal shift
    int y;             ///< Vertical shift
} IntMotionVector;

typedef struct {
    double x;             ///< Horizontal shift
    double y;             ///< Vertical shift
} MotionVector;

typedef struct {
    MotionVector vector;  ///< Motion vector
    double angle;         ///< Angle of rotation
    double zoom;          ///< Zoom percentage
} Transform;

#if CONFIG_OPENCL

typedef struct {
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel_luma;
    cl_kernel kernel_chroma;
    int in_plane_size[8];
    int out_plane_size[8];
    int plane_num;
    cl_mem cl_inbuf;
    size_t cl_inbuf_size;
    cl_mem cl_outbuf;
    size_t cl_outbuf_size;
} DeshakeOpenclContext;

#endif

typedef struct {
    const AVClass *class;
    AVFrame *ref;              ///< Previous frame
    int rx;                    ///< Maximum horizontal shift
    int ry;                    ///< Maximum vertical shift
    int edge;                  ///< Edge fill method
    int blocksize;             ///< Size of blocks to compare
    int contrast;              ///< Contrast threshold
    int search;                ///< Motion search method
    AVCodecContext *avctx;
    DSPContext c;              ///< Context providing optimized SAD methods
    Transform last;            ///< Transform from last frame
    int refcount;              ///< Number of reference frames (defines averaging window)
    FILE *fp;
    Transform avg;
    int cw;                    ///< Crop motion search to this box
    int ch;
    int cx;
    int cy;
    char *filename;            ///< Motion search detailed log filename
    int opencl;
#if CONFIG_OPENCL
    DeshakeOpenclContext opencl_ctx;
#endif
    int (* transform)(AVFilterContext *ctx, int width, int height, int cw, int ch,
                      const float *matrix_y, const float *matrix_uv, enum InterpolateMethod interpolate,
                      enum FillMethod fill, AVFrame *in, AVFrame *out);
} DeshakeContext;

#endif /* AVFILTER_DESHAKE_H */
