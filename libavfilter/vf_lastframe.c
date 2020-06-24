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
 * filter for selecting and holding the last frame passed in the filterchain
 */

#include "libavutil/opt.h"
#include "internal.h"
#include "libavutil/avassert.h"
#include "avfilter.h"
#include "filters.h"
#include "formats.h"

typedef struct LastFrameContext {
    const AVClass *class;
    int64_t duration;
    int64_t frame_remaining;  // number of frames to hold
    AVFrame *last_frame;  // store the last frame

    int64_t pts;  // current pts of the newly freezed stream
    int eof;  // whether approach to the end of the stream
} LastFrameContext;

#define OFFSET(x) offsetof(LastFrameContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption lastframe_options[] = {
    { "duration", "set the duration to hold the frame (default is one second)",
      OFFSET(duration), AV_OPT_TYPE_DURATION, {.i64=0}, 0, INT64_MAX, FLAGS},
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

    // if the stream has not ended, read from it
    if (!s->eof) {
        ret = ff_inlink_consume_frame(inlink, &frame);
        if (ret < 0)
            return ret;
        if (ret > 0) {
          // if read the frame successfully, update the last_frame attribute
          s->last_frame = av_frame_clone(frame);
          av_frame_free(&frame);
        }
    }

    // if a status change is detected, update the status
    if (!s->eof && ff_inlink_acknowledge_status(inlink, &status, &pts)) {
        if (status == AVERROR_EOF) {
            s->eof = 1;
        }
    }

    // if no frame remaining in the FIFO queue
    if (s->eof) {
        // if all extra frames have been outputted, inform the status change
        if (!s->frame_remaining) {
            ff_outlink_set_status(outlink, AVERROR_EOF, s->pts);
            return 0;
        } else {
          frame = av_frame_clone(s->last_frame);
          if (!frame)
              return AVERROR(ENOMEM);
          frame->pts = s->pts;
          s->pts += av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
          if (s->frame_remaining > 0)
              s->frame_remaining--;
          return ff_filter_frame(outlink, frame);
        }
    }
    // if the stream has not ended or there are frames left.
    if (!s->eof || s->frame_remaining > 0)
        FF_FILTER_FORWARD_WANTED(outlink, inlink);

    return FFERROR_NOT_READY;
}

// currently we just support the most common YUV420, can add more if needed
static int query_formats(AVFilterContext *ctx) {
    static const enum AVPixelFormat formats[] = {
        AV_PIX_FMT_RGB24,
        AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_NONE
    };

    return ff_set_common_formats(ctx, ff_make_format_list(formats));
}

static av_cold void uninit(AVFilterContext *ctx) {
    LastFrameContext *s = ctx->priv;

    av_frame_free(&s->last_frame);
}

static int config_input(AVFilterLink *inlink) {
    AVFilterContext *ctx = inlink->dst;
    LastFrameContext *s = ctx->priv;

    if (s->duration) {
      s->frame_remaining = av_rescale_q(s->duration, inlink->frame_rate, av_inv_q(AV_TIME_BASE_Q));
    } else {
      // the default number of frame is the frame_rate
      s->frame_remaining = av_rescale_q(AV_TIME_BASE, inlink->frame_rate, av_inv_q(AV_TIME_BASE_Q));
    }
    return 0;
}

static const AVFilterPad avfilter_vf_lastframe_inputs[] = {
  {
    .name = "default",
    .type = AVMEDIA_TYPE_VIDEO,
    .config_props = config_input,
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
    .description   = NULL_IF_CONFIG_SMALL("Select and hold the last frame of a video to pass in output."),
    .priv_size     = sizeof(LastFrameContext),
    .priv_class    = &lastframe_class,
    .query_formats = query_formats,
    .activate      = activate,
    .uninit        = uninit,
    .inputs        = avfilter_vf_lastframe_inputs,
    .outputs       = avfilter_vf_lastframe_outputs,
};
