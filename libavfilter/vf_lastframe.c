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
    int64_t pts;
    int eof;
    int frame_remaining;

} LastFrameContext;

#define OFFSET(x) offsetof(LastFrameContext, x)

static const AVOption lastframe_options[] = {
    { NULL }
};

AVFILTER_DEFINE_CLASS(lastframe);

// static int activate(AVFilterContext *ctx) {
//     AVFilterLink *inlink = ctx->inputs[0];
//     AVFilterLink *outlink = ctx->outputs[0];
//     LastFrameContext *s = ctx->priv;
//     AVFrame *frame = NULL;
//     int ret, status;
//     int64_t pts;

//     FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);
//     printf("ff_outlink_frame_wanted? %d\n", ff_outlink_frame_wanted(outlink));

//     if (s->eof && s->frame_remaining > 0) {
//       printf("endpoint2\n");
//       s->frame_remaining--;
//       frame = av_frame_clone(s->last_frame);
//       frame->pts = s->pts;
//       s->pts += av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
//       printf("\nremaining frame info: remaining %d, pts %d\n", s->frame_remaining, frame->pts);
//       return ff_filter_frame(outlink, frame);
//       // ff_outlink_set_status(outlink, AVERROR_EOF, pts);
//       // return 0;
//     } else if (s->eof) {
//       printf("endpoint3\n");
//       // pts = av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
//       printf("\nComplete\n", s->frame_remaining);
//       s->last_frame = NULL;
//       ff_outlink_set_status(outlink, AVERROR_EOF, s->pts);
//       return 0;
//     }

//     if (!s->eof) { // 如果流还没有结束
//         ret = ff_inlink_consume_frame(inlink, &frame);
//         if (ret < 0) 
//             return ret;
//         if (ret > 0) {
//             printf("\nframe info: pts %d\n", frame->pts);
//             pts = av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
//             printf("\n-- single pts %d\n", pts);
//             s->last_frame = av_frame_clone(frame);
//             // av_frame_free(&frame);
//             // return 0;
//             return ff_filter_frame(outlink, frame);
//         }

//         if (ff_inlink_acknowledge_status(inlink, &status, &pts) && status == AVERROR_EOF) {
//             s->eof = 1;
//             s->pts = pts;
//             // return ff_filter_frame(outlink, frame);
//             // pts = av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
//             // printf("\n-- last frame pts %d\n", s->last_frame->pts);
//             printf("\n\n\n\n\nthe stream ends: pts %d\n\n\n\n\n", pts);
//             // ff_outlink_set_status(outlink, status, pts);
//             return 0;
//         }
//     }
    

//     printf("endpoint8\n");
//     // FF_FILTER_FORWARD_WANTED(outlink, inlink);
//     // return FFERROR_NOT_READY;
//     // FF_FILTER_FORWARD_WANTED(outlink, inlink);
//     // printf("endpoint9\n");
//     // return FFERROR_NOT_READY;
//     return 0;
// }



//     if (!s->eof) { // 如果流还没有结束
//         ret = ff_inlink_consume_frame(inlink, &frame);
//         if (ret < 0) 
//             return ret;
//         if (ret > 0) {
//             printf("\nframe info: pts %d\n", frame->pts);
//             pts = av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
//             printf("\n-- single pts %d\n", pts);
//             s->last_frame = av_frame_clone(frame);
//             // av_frame_free(&frame);
//             // return 0;
//             return ff_filter_frame(outlink, frame);
//         }

//         if (ff_inlink_acknowledge_status(inlink, &status, &pts) && status == AVERROR_EOF) {
//             s->eof = 1;
//             s->pts = pts;
//             // return ff_filter_frame(outlink, frame);
//             // pts = av_rescale_q(1, av_inv_q(outlink->frame_rate), outlink->time_base);
//             // printf("\n-- last frame pts %d\n", s->last_frame->pts);
//             printf("\n\n\n\n\nthe stream ends: pts %d\n\n\n\n\n", pts);
//             // ff_outlink_set_status(outlink, status, pts);
//             return 0;
//         }
//     }
    

//     printf("endpoint8\n");
//     // FF_FILTER_FORWARD_WANTED(outlink, inlink);
//     // return FFERROR_NOT_READY;
//     // FF_FILTER_FORWARD_WANTED(outlink, inlink);
//     // printf("endpoint9\n");
//     // return FFERROR_NOT_READY;
//     return 0;
// }



static int activate(AVFilterContext *ctx) {
    AVFilterLink *inlink = ctx->inputs[0];
    AVFilterLink *outlink = ctx->outputs[0];
    LastFrameContext *s = ctx->priv;
    AVFrame *frame = NULL;
    int ret, status;
    int64_t pts;

    FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);
    printf("ff_outlink_frame_wanted? %d\n\n", ff_outlink_frame_wanted(outlink));
 
    if (!s->eof) {
        ret = ff_inlink_consume_frame(inlink, &frame);
        if (ret < 0)
            return ret;
        if (ret > 0) {
          printf("\nframe info: pts %d\n", frame->pts);
          s->last_frame = av_frame_clone(frame);
          frame->pts = 0;
          // return ff_filter_frame(outlink, frame);
          av_frame_free(&frame);
          // return 0;
        }
    }
    
    // 如果流还没有结束，但是status change发生了，流结束了
    if (!s->eof && ff_inlink_acknowledge_status(inlink, &status, &pts)) {
        if (status == AVERROR_EOF) {
            printf("\n\n\n\n\nthe stream ends: pts %d\n\n\n\n\n", pts);
            printf("self: pts %d\n\n\n\n\n", s->pts);
            s->eof = 1; // 更新说明结束了
            // s->pts += pts; 
        }
    }

    printf("\n\n-------- in_frame_count_out %d, out_frame_count_out %d \neof: %d, s->pts %d, s->frame_remaining %d\n",
    inlink->frame_count_out, outlink->frame_count_out,s->eof, s->pts, s->frame_remaining);
    // 如果视频流结束了，但是还需要加frame
    if (s->eof) {
        // 如果结尾已经加完了，告诉说结束
        if (!s->frame_remaining) {
            printf("completed!\n");
            ff_outlink_set_status(outlink, AVERROR_EOF, 64000);
            return 0;
        } else {
          printf("s->pts %d, s->frame_remaining %d\n", s->pts, s->frame_remaining);
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
    // 如果视频流还没有结束，或者还有需要加的frames
    if (!s->eof || s->frame_remaining > 0) 
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

static av_cold void uninit(AVFilterContext *ctx)
{
    LastFrameContext *s = ctx->priv;

    av_frame_free(&s->last_frame);
}

static int config_input(AVFilterLink *inlink)
{
    AVFilterContext *ctx = inlink->dst;
    LastFrameContext *s = ctx->priv;

    s->frame_remaining = 50;

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
    .description   = NULL_IF_CONFIG_SMALL("Select the last frame of a video to pass in output."),
    .priv_size     = sizeof(LastFrameContext),
    .priv_class    = &lastframe_class,
    .query_formats = query_formats,
    .activate      = activate,
    .uninit        = uninit,
    .inputs        = avfilter_vf_lastframe_inputs,
    .outputs       = avfilter_vf_lastframe_outputs,
};
