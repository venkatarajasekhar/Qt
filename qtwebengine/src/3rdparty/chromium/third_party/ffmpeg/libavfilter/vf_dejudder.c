/*
 * Copyright (c) 2014 Nicholas Robbins
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

/**
 * @file
 * remove judder in video stream
 *
 * Algorithm:
 *    - If the old packets had PTS of old_pts[i]. Replace these with new
 *      value based on the running average of the last n=cycle frames. So
 *
 *      new_pts[i] = Sum(k=i-n+1, i, old_pts[k])/n
 *                        + (old_pts[i]-old_pts[i-n])*(n-1)/2n
 *
 *      For any repeating pattern of length n of judder this will produce
 *      an even progression of PTS's.
 *
 *    - In order to avoid calculating this sum ever frame, a running tally
 *      is maintained in ctx->new_pts. Each frame the new term at the start
 *      of the sum is added, the one and the end is removed, and the offset
 *      terms (second line in formula above) are recalculated.
 *
 *    - To aid in this a ringbuffer of the last n-2 PTS's is maintained in
 *      ctx->ringbuff. With the indices of the first two and last two entries
 *      stored in i1, i2, i3, & i4.
 *
 *    - To ensure that the new PTS's are integers, time_base is divided
 *      by 2n. This removes the division in the new_pts calculation.
 *
 *    - frame_rate is also multiplied by 2n to allow the frames to fall
 *      where they may in what may now be a VFR output. This produces more
 *      even output then setting frame_rate=1/0 in practice.
 */

#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "avfilter.h"
#include "internal.h"
#include "video.h"

typedef struct {
    const AVClass *class;
    int64_t *ringbuff;
    int i1, i2, i3, i4;
    int64_t new_pts;
    int start_count;

    /* options */
    int cycle;
} DejudderContext;

#define OFFSET(x) offsetof(DejudderContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM | AV_OPT_FLAG_VIDEO_PARAM

static const AVOption dejudder_options[] = {
    {"cycle", "set the length of the cycle to use for dejuddering",
        OFFSET(cycle), AV_OPT_TYPE_INT, {.i64 = 4}, 2, 240, .flags = FLAGS},
    {NULL}
};

AVFILTER_DEFINE_CLASS(dejudder);

static int config_out_props(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    DejudderContext *dj = ctx->priv;
    AVFilterLink *inlink = outlink->src->inputs[0];

    outlink->time_base = av_mul_q(inlink->time_base, av_make_q(1, 2 * dj->cycle));
    outlink->frame_rate = av_mul_q(inlink->frame_rate, av_make_q(2 * dj->cycle, 1));

    av_log(ctx, AV_LOG_VERBOSE, "cycle:%d\n", dj->cycle);

    return 0;
}

static av_cold int dejudder_init(AVFilterContext *ctx)
{
    DejudderContext *dj = ctx->priv;

    dj->ringbuff = av_mallocz(sizeof(*dj->ringbuff) * (dj->cycle+2));
    if (!dj->ringbuff)
        return AVERROR(ENOMEM);

    dj->new_pts = 0;
    dj->i1 = 0;
    dj->i2 = 1;
    dj->i3 = 2;
    dj->i4 = 3;
    dj->start_count = dj->cycle + 2;

    return 0;
}

static av_cold void dejudder_uninit(AVFilterContext *ctx)
{
    DejudderContext *dj = ctx->priv;

    av_freep(&(dj->ringbuff));
}

static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    int k;
    AVFilterContext *ctx  = inlink->dst;
    AVFilterLink *outlink = ctx->outputs[0];
    DejudderContext *dj   = ctx->priv;
    int64_t *judbuff      = dj->ringbuff;
    int64_t next_pts      = frame->pts;
    int64_t offset;

    if (next_pts == AV_NOPTS_VALUE)
        return ff_filter_frame(outlink, frame);

    if (dj->start_count) {
        dj->start_count--;
        dj->new_pts = next_pts * 2 * dj->cycle;
    } else {
        if (next_pts < judbuff[dj->i2]) {
            offset = next_pts + judbuff[dj->i3] - judbuff[dj->i4] - judbuff[dj->i1];
            for (k = 0; k < dj->cycle + 2; k++)
                judbuff[k] += offset;
        }
        dj->new_pts += (dj->cycle - 1) * (judbuff[dj->i3] - judbuff[dj->i1])
                    + (dj->cycle + 1) * (next_pts - judbuff[dj->i4]);
    }

    judbuff[dj->i2] = next_pts;
    dj->i1 = dj->i2;
    dj->i2 = dj->i3;
    dj->i3 = dj->i4;
    dj->i4 = (dj->i4 + 1) % (dj->cycle + 2);

    frame->pts = dj->new_pts;

    for (k = 0; k < dj->cycle + 2; k++)
        av_log(ctx, AV_LOG_DEBUG, "%"PRId64"\t", judbuff[k]);
    av_log(ctx, AV_LOG_DEBUG, "next=%"PRId64", new=%"PRId64"\n", next_pts, frame->pts);

    return ff_filter_frame(outlink, frame);
}

static const AVFilterPad dejudder_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad dejudder_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
        .config_props = config_out_props,
    },
    { NULL }
};

AVFilter ff_vf_dejudder = {
    .name        = "dejudder",
    .description = NULL_IF_CONFIG_SMALL("Remove judder produced by pullup."),
    .priv_size   = sizeof(DejudderContext),
    .priv_class  = &dejudder_class,
    .inputs      = dejudder_inputs,
    .outputs     = dejudder_outputs,
    .init        = dejudder_init,
    .uninit      = dejudder_uninit,
};
