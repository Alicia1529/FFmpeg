/*
 * Copyright (c) 2020 Alicia Luo
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
 * filter for selecting the last frame passes in the filterchain
 */

#include "libavutil/opt.h"
#include "internal.h"
#include "libavutil/avassert.h"
#include "avfilter.h"
#include "filters.h"
#include "formats.h"

typedef struct LastFrameContext {
    const AVClass *class;
    AVFrame *last_frame;
} LastFrameContext;

#define OFFSET(x) offsetof(LastFrameContext, x)

static const AVOption lastframe_options[] = {
    { NULL }
};

AVFILTER_DEFINE_CLASS(lastframe);

static int activate(AVFilterContext *ctx) {
    AVFilterLink *inlink = ctx->inputs[0];
    AVFilterLink *outlink = ctx->outputs[0];
    LastFrameContext *s = ctx->priv;
    AVFrame *frame = NULL;
    int ret, status;
    int64_t pts;

    FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);

    ret = ff_inlink_consume_frame(inlink, &frame);
    if (ret < 0) 
        return ret;
    if (ret > 0) {
        s->last_frame = av_frame_clone(frame);
        return ff_filter_frame(outlink, frame);
    }

    // 如果status change发生了，说明流结束了
    if (ff_inlink_acknowledge_status(inlink, &status, &pts) && status == AVERROR_EOF) {
        ff_outlink_set_status(outlink, status, pts);
        return 0;
    }

    // FF_FILTER_FORWARD_WANTED(outlink, inlink);
    // return FFERROR_NOT_READY;
    FF_FILTER_FORWARD_WANTED(outlink, inlink);

    return FFERROR_NOT_READY;
}

//currently we just support the most common YUV420, can add more if needed
static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat formats[] = {
        AV_PIX_FMT_RGB24,
        AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_NONE
    };

    return ff_set_common_formats(ctx, ff_make_format_list(formats));
}

static const AVFilterPad avfilter_vf_lastframe_inputs[] = {
  {
    .name = "default",
    .type = AVMEDIA_TYPE_VIDEO,
    // .filter_frame = filter_frame,
  },
  {NULL}
};

static const AVFilterPad avfilter_vf_lastframe_outputs[] = {
  {
    .name = "default",
    .type = AVMEDIA_TYPE_VIDEO,
  },
  {NULL}
};

AVFilter ff_vf_lastframe = {
    .name          = "lastframe",
    .description   = NULL_IF_CONFIG_SMALL("Select the last frame of a video to pass in output."),
    .priv_size     = sizeof(LastFrameContext),
    .priv_class    = &lastframe_class,
    .query_formats = query_formats,
    .activate      = activate,
    .inputs        = avfilter_vf_lastframe_inputs,
    .outputs       = avfilter_vf_lastframe_outputs,
};
