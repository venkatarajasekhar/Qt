/*
 * Copyright (c) 2013 Georg Martius <georg dot martius at web dot de>
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

#define DEFAULT_RESULT_NAME     "transforms.trf"

#include <vid.stab/libvidstab.h>

#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "avfilter.h"
#include "internal.h"

#include "vidstabutils.h"

typedef struct {
    const AVClass *class;

    VSMotionDetect md;
    VSMotionDetectConfig conf;

    char *result;
    FILE *f;
} StabData;


#define OFFSET(x) offsetof(StabData, x)
#define OFFSETC(x) (offsetof(StabData, conf)+offsetof(VSMotionDetectConfig, x))
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption vidstabdetect_options[] = {
    {"result",      "path to the file used to write the transforms",                 OFFSET(result),             AV_OPT_TYPE_STRING, {.str = DEFAULT_RESULT_NAME}, .flags = FLAGS},
    {"shakiness",   "how shaky is the video and how quick is the camera?"
                    " 1: little (fast) 10: very strong/quick (slow)",                OFFSETC(shakiness),         AV_OPT_TYPE_INT,    {.i64 = 5},      1,  10, FLAGS},
    {"accuracy",    "(>=shakiness) 1: low 15: high (slow)",                          OFFSETC(accuracy),          AV_OPT_TYPE_INT,    {.i64 = 15},     1,  15, FLAGS},
    {"stepsize",    "region around minimum is scanned with 1 pixel resolution",      OFFSETC(stepSize),          AV_OPT_TYPE_INT,    {.i64 = 6},      1,  32, FLAGS},
    {"mincontrast", "below this contrast a field is discarded (0-1)",                OFFSETC(contrastThreshold), AV_OPT_TYPE_DOUBLE, {.dbl = 0.25}, 0.0, 1.0, FLAGS},
    {"show",        "0: draw nothing; 1,2: show fields and transforms",              OFFSETC(show),              AV_OPT_TYPE_INT,    {.i64 = 0},      0,   2, FLAGS},
    {"tripod",      "virtual tripod mode (if >0): motion is compared to a reference"
                    " reference frame (frame # is the value)",                       OFFSETC(virtualTripod),     AV_OPT_TYPE_INT,    {.i64 = 0}, 0, INT_MAX, FLAGS},
    {NULL}
};

AVFILTER_DEFINE_CLASS(vidstabdetect);

static av_cold int init(AVFilterContext *ctx)
{
    StabData *sd = ctx->priv;
    vs_set_mem_and_log_functions();
    sd->class = &vidstabdetect_class;
    av_log(ctx, AV_LOG_VERBOSE, "vidstabdetect filter: init %s\n", LIBVIDSTAB_VERSION);
    return 0;
}

static av_cold void uninit(AVFilterContext *ctx)
{
    StabData *sd = ctx->priv;
    VSMotionDetect *md = &(sd->md);

    if (sd->f) {
        fclose(sd->f);
        sd->f = NULL;
    }

    vsMotionDetectionCleanup(md);
}

static int query_formats(AVFilterContext *ctx)
{
    // If you add something here also add it in vidstabutils.c
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_YUV444P,  AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUV411P,  AV_PIX_FMT_YUV410P, AV_PIX_FMT_YUVA420P,
        AV_PIX_FMT_YUV440P,  AV_PIX_FMT_GRAY8,
        AV_PIX_FMT_RGB24, AV_PIX_FMT_BGR24, AV_PIX_FMT_RGBA,
        AV_PIX_FMT_NONE
    };

    ff_set_common_formats(ctx, ff_make_format_list(pix_fmts));
    return 0;
}

static int config_input(AVFilterLink *inlink)
{
    AVFilterContext *ctx = inlink->dst;
    StabData *sd = ctx->priv;

    VSMotionDetect* md = &(sd->md);
    VSFrameInfo fi;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);

    vsFrameInfoInit(&fi, inlink->w, inlink->h, av_2_vs_pixel_format(ctx, inlink->format));
    if (fi.bytesPerPixel != av_get_bits_per_pixel(desc)/8) {
        av_log(ctx, AV_LOG_ERROR, "pixel-format error: wrong bits/per/pixel, please report a BUG");
        return AVERROR(EINVAL);
    }
    if (fi.log2ChromaW != desc->log2_chroma_w) {
        av_log(ctx, AV_LOG_ERROR, "pixel-format error: log2_chroma_w, please report a BUG");
        return AVERROR(EINVAL);
    }

    if (fi.log2ChromaH != desc->log2_chroma_h) {
        av_log(ctx, AV_LOG_ERROR, "pixel-format error: log2_chroma_h, please report a BUG");
        return AVERROR(EINVAL);
    }

    // set values that are not initialized by the options
    sd->conf.algo     = 1;
    sd->conf.modName  = "vidstabdetect";
    if (vsMotionDetectInit(md, &sd->conf, &fi) != VS_OK) {
        av_log(ctx, AV_LOG_ERROR, "initialization of Motion Detection failed, please report a BUG");
        return AVERROR(EINVAL);
    }

    vsMotionDetectGetConfig(&sd->conf, md);
    av_log(ctx, AV_LOG_INFO, "Video stabilization settings (pass 1/2):\n");
    av_log(ctx, AV_LOG_INFO, "     shakiness = %d\n", sd->conf.shakiness);
    av_log(ctx, AV_LOG_INFO, "      accuracy = %d\n", sd->conf.accuracy);
    av_log(ctx, AV_LOG_INFO, "      stepsize = %d\n", sd->conf.stepSize);
    av_log(ctx, AV_LOG_INFO, "   mincontrast = %f\n", sd->conf.contrastThreshold);
    av_log(ctx, AV_LOG_INFO, "        tripod = %d\n", sd->conf.virtualTripod);
    av_log(ctx, AV_LOG_INFO, "          show = %d\n", sd->conf.show);
    av_log(ctx, AV_LOG_INFO, "        result = %s\n", sd->result);

    sd->f = fopen(sd->result, "w");
    if (sd->f == NULL) {
        av_log(ctx, AV_LOG_ERROR, "cannot open transform file %s\n", sd->result);
        return AVERROR(EINVAL);
    } else {
        if (vsPrepareFile(md, sd->f) != VS_OK) {
            av_log(ctx, AV_LOG_ERROR, "cannot write to transform file %s\n", sd->result);
            return AVERROR(EINVAL);
        }
    }
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    StabData *sd = ctx->priv;
    VSMotionDetect *md = &(sd->md);
    LocalMotions localmotions;

    AVFilterLink *outlink = inlink->dst->outputs[0];
    VSFrame frame;
    int plane;

    if (sd->conf.show > 0 && !av_frame_is_writable(in))
        av_frame_make_writable(in);

    for (plane = 0; plane < md->fi.planes; plane++) {
        frame.data[plane] = in->data[plane];
        frame.linesize[plane] = in->linesize[plane];
    }
    if (vsMotionDetection(md, &localmotions, &frame) != VS_OK) {
        av_log(ctx, AV_LOG_ERROR, "motion detection failed");
        return AVERROR(AVERROR_EXTERNAL);
    } else {
        if (vsWriteToFile(md, sd->f, &localmotions) != VS_OK) {
            av_log(ctx, AV_LOG_ERROR, "cannot write to transform file");
            return AVERROR(errno);
        }
        vs_vector_del(&localmotions);
    }

    return ff_filter_frame(outlink, in);
}

static const AVFilterPad avfilter_vf_vidstabdetect_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
        .config_props = config_input,
    },
    { NULL }
};

static const AVFilterPad avfilter_vf_vidstabdetect_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_vidstabdetect = {
    .name          = "vidstabdetect",
    .description   = NULL_IF_CONFIG_SMALL("Extract relative transformations, "
                                          "pass 1 of 2 for stabilization "
                                          "(see vidstabtransform for pass 2)."),
    .priv_size     = sizeof(StabData),
    .init          = init,
    .uninit        = uninit,
    .query_formats = query_formats,
    .inputs        = avfilter_vf_vidstabdetect_inputs,
    .outputs       = avfilter_vf_vidstabdetect_outputs,
    .priv_class    = &vidstabdetect_class,
};
