/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//  This is an example demonstrating how to implement a multi-layer VPx
//  encoding scheme based on temporal scalability for video applications
//  that benefit from a scalable bitstream.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VPX_CODEC_DISABLE_COMPAT 1
#include "./vpx_config.h"
#include "vpx_ports/vpx_timer.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_encoder.h"

#include "./tools_common.h"
#include "./video_writer.h"

static const char *exec_name;

void usage_exit() {
  exit(EXIT_FAILURE);
}

static int mode_to_num_layers[12] = {1, 2, 2, 3, 3, 3, 3, 5, 2, 3, 3, 3};

// For rate control encoding stats.
struct RateControlMetrics {
  // Number of input frames per layer.
  int layer_input_frames[VPX_TS_MAX_LAYERS];
  // Total (cumulative) number of encoded frames per layer.
  int layer_tot_enc_frames[VPX_TS_MAX_LAYERS];
  // Number of encoded non-key frames per layer.
  int layer_enc_frames[VPX_TS_MAX_LAYERS];
  // Framerate per layer layer (cumulative).
  double layer_framerate[VPX_TS_MAX_LAYERS];
  // Target average frame size per layer (per-frame-bandwidth per layer).
  double layer_pfb[VPX_TS_MAX_LAYERS];
  // Actual average frame size per layer.
  double layer_avg_frame_size[VPX_TS_MAX_LAYERS];
  // Average rate mismatch per layer (|target - actual| / target).
  double layer_avg_rate_mismatch[VPX_TS_MAX_LAYERS];
  // Actual encoding bitrate per layer (cumulative).
  double layer_encoding_bitrate[VPX_TS_MAX_LAYERS];
};

// Note: these rate control metrics assume only 1 key frame in the
// sequence (i.e., first frame only). So for temporal pattern# 7
// (which has key frame for every frame on base layer), the metrics
// computation will be off/wrong.
// TODO(marpan): Update these metrics to account for multiple key frames
// in the stream.
static void set_rate_control_metrics(struct RateControlMetrics *rc,
                                     vpx_codec_enc_cfg_t *cfg) {
  unsigned int i = 0;
  // Set the layer (cumulative) framerate and the target layer (non-cumulative)
  // per-frame-bandwidth, for the rate control encoding stats below.
  const double framerate = cfg->g_timebase.den / cfg->g_timebase.num;
  rc->layer_framerate[0] = framerate / cfg->ts_rate_decimator[0];
  rc->layer_pfb[0] = 1000.0 * cfg->ts_target_bitrate[0] /
      rc->layer_framerate[0];
  for (i = 0; i < cfg->ts_number_layers; ++i) {
    if (i > 0) {
      rc->layer_framerate[i] = framerate / cfg->ts_rate_decimator[i];
      rc->layer_pfb[i] = 1000.0 *
          (cfg->ts_target_bitrate[i] - cfg->ts_target_bitrate[i - 1]) /
          (rc->layer_framerate[i] - rc->layer_framerate[i - 1]);
    }
    rc->layer_input_frames[i] = 0;
    rc->layer_enc_frames[i] = 0;
    rc->layer_tot_enc_frames[i] = 0;
    rc->layer_encoding_bitrate[i] = 0.0;
    rc->layer_avg_frame_size[i] = 0.0;
    rc->layer_avg_rate_mismatch[i] = 0.0;
  }
}

static void printout_rate_control_summary(struct RateControlMetrics *rc,
                                          vpx_codec_enc_cfg_t *cfg,
                                          int frame_cnt) {
  unsigned int i = 0;
  int tot_num_frames = 0;
  printf("Total number of processed frames: %d\n\n", frame_cnt -1);
  printf("Rate control layer stats for %d layer(s):\n\n",
      cfg->ts_number_layers);
  for (i = 0; i < cfg->ts_number_layers; ++i) {
    const int num_dropped = (i > 0) ?
        (rc->layer_input_frames[i] - rc->layer_enc_frames[i]) :
        (rc->layer_input_frames[i] - rc->layer_enc_frames[i] - 1);
    tot_num_frames += rc->layer_input_frames[i];
    rc->layer_encoding_bitrate[i] = 0.001 * rc->layer_framerate[i] *
        rc->layer_encoding_bitrate[i] / tot_num_frames;
    rc->layer_avg_frame_size[i] = rc->layer_avg_frame_size[i] /
        rc->layer_enc_frames[i];
    rc->layer_avg_rate_mismatch[i] = 100.0 * rc->layer_avg_rate_mismatch[i] /
        rc->layer_enc_frames[i];
    printf("For layer#: %d \n", i);
    printf("Bitrate (target vs actual): %d %f \n", cfg->ts_target_bitrate[i],
           rc->layer_encoding_bitrate[i]);
    printf("Average frame size (target vs actual): %f %f \n", rc->layer_pfb[i],
           rc->layer_avg_frame_size[i]);
    printf("Average rate_mismatch: %f \n", rc->layer_avg_rate_mismatch[i]);
    printf("Number of input frames, encoded (non-key) frames, "
        "and perc dropped frames: %d %d %f \n", rc->layer_input_frames[i],
        rc->layer_enc_frames[i],
        100.0 * num_dropped / rc->layer_input_frames[i]);
    printf("\n");
  }
  if ((frame_cnt - 1) != tot_num_frames)
    die("Error: Number of input frames not equal to output! \n");
}

// Temporal scaling parameters:
// NOTE: The 3 prediction frames cannot be used interchangeably due to
// differences in the way they are handled throughout the code. The
// frames should be allocated to layers in the order LAST, GF, ARF.
// Other combinations work, but may produce slightly inferior results.
static void set_temporal_layer_pattern(int layering_mode,
                                       vpx_codec_enc_cfg_t *cfg,
                                       int *layer_flags,
                                       int *flag_periodicity) {
  switch (layering_mode) {
    case 0: {
      // 1-layer.
      int ids[1] = {0};
      cfg->ts_periodicity = 1;
      *flag_periodicity = 1;
      cfg->ts_number_layers = 1;
      cfg->ts_rate_decimator[0] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // Update L only.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF;
      break;
    }
    case 1: {
      // 2-layers, 2-frame period.
      int ids[2] = {0, 1};
      cfg->ts_periodicity = 2;
      *flag_periodicity = 2;
      cfg->ts_number_layers = 2;
      cfg->ts_rate_decimator[0] = 2;
      cfg->ts_rate_decimator[1] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
#if 1
      // 0=L, 1=GF, Intra-layer prediction enabled.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF;
      layer_flags[1] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_REF_ARF;
#else
       // 0=L, 1=GF, Intra-layer prediction disabled.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF;
      layer_flags[1] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_REF_LAST;
#endif
      break;
    }
    case 2: {
      // 2-layers, 3-frame period.
      int ids[3] = {0, 1, 1};
      cfg->ts_periodicity = 3;
      *flag_periodicity = 3;
      cfg->ts_number_layers = 2;
      cfg->ts_rate_decimator[0] = 3;
      cfg->ts_rate_decimator[1] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, Intra-layer prediction enabled.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[1] =
      layer_flags[2] = VP8_EFLAG_NO_REF_GF  | VP8_EFLAG_NO_REF_ARF |
          VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST;
      break;
    }
    case 3: {
      // 3-layers, 6-frame period.
      int ids[6] = {0, 2, 2, 1, 2, 2};
      cfg->ts_periodicity = 6;
      *flag_periodicity = 6;
      cfg->ts_number_layers = 3;
      cfg->ts_rate_decimator[0] = 6;
      cfg->ts_rate_decimator[1] = 3;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF, Intra-layer prediction enabled.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[3] = VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_UPD_LAST;
      layer_flags[1] =
      layer_flags[2] =
      layer_flags[4] =
      layer_flags[5] = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_LAST;
      break;
    }
    case 4: {
      // 3-layers, 4-frame period.
      int ids[4] = {0, 2, 1, 2};
      cfg->ts_periodicity = 4;
      *flag_periodicity = 4;
      cfg->ts_number_layers = 3;
      cfg->ts_rate_decimator[0] = 4;
      cfg->ts_rate_decimator[1] = 2;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF, Intra-layer prediction disabled.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[2] = VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF |
          VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST;
      layer_flags[1] =
      layer_flags[3] = VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      break;
    }
    case 5: {
      // 3-layers, 4-frame period.
      int ids[4] = {0, 2, 1, 2};
      cfg->ts_periodicity = 4;
      *flag_periodicity = 4;
      cfg->ts_number_layers     = 3;
      cfg->ts_rate_decimator[0] = 4;
      cfg->ts_rate_decimator[1] = 2;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF, Intra-layer prediction enabled in layer 1, disabled
      // in layer 2.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[2] = VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_ARF;
      layer_flags[1] =
      layer_flags[3] = VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      break;
    }
    case 6: {
      // 3-layers, 4-frame period.
      int ids[4] = {0, 2, 1, 2};
      cfg->ts_periodicity = 4;
      *flag_periodicity = 4;
      cfg->ts_number_layers = 3;
      cfg->ts_rate_decimator[0] = 4;
      cfg->ts_rate_decimator[1] = 2;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF, Intra-layer prediction enabled.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[2] = VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_ARF;
      layer_flags[1] =
      layer_flags[3] = VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF;
      break;
    }
    case 7: {
      // NOTE: Probably of academic interest only.
      // 5-layers, 16-frame period.
      int ids[16] = {0, 4, 3, 4, 2, 4, 3, 4, 1, 4, 3, 4, 2, 4, 3, 4};
      cfg->ts_periodicity = 16;
      *flag_periodicity = 16;
      cfg->ts_number_layers = 5;
      cfg->ts_rate_decimator[0] = 16;
      cfg->ts_rate_decimator[1] = 8;
      cfg->ts_rate_decimator[2] = 4;
      cfg->ts_rate_decimator[3] = 2;
      cfg->ts_rate_decimator[4] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      layer_flags[0]  = VPX_EFLAG_FORCE_KF;
      layer_flags[1]  =
      layer_flags[3]  =
      layer_flags[5]  =
      layer_flags[7]  =
      layer_flags[9]  =
      layer_flags[11] =
      layer_flags[13] =
      layer_flags[15] = VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF;
      layer_flags[2]  =
      layer_flags[6]  =
      layer_flags[10] =
      layer_flags[14] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_GF;
      layer_flags[4] =
      layer_flags[12] = VP8_EFLAG_NO_REF_LAST | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[8]  = VP8_EFLAG_NO_REF_LAST | VP8_EFLAG_NO_REF_GF;
      break;
    }
    case 8: {
      // 2-layers, with sync point at first frame of layer 1.
      int ids[2] = {0, 1};
      cfg->ts_periodicity = 2;
      *flag_periodicity = 8;
      cfg->ts_number_layers = 2;
      cfg->ts_rate_decimator[0] = 2;
      cfg->ts_rate_decimator[1] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF.
      // ARF is used as predictor for all frames, and is only updated on
      // key frame. Sync point every 8 frames.

      // Layer 0: predict from L and ARF, update L and G.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_UPD_ARF;
      // Layer 1: sync point: predict from L and ARF, and update G.
      layer_flags[1] = VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_ARF;
      // Layer 0, predict from L and ARF, update L.
      layer_flags[2] = VP8_EFLAG_NO_REF_GF  | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF;
      // Layer 1: predict from L, G and ARF, and update G.
      layer_flags[3] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_ENTROPY;
      // Layer 0.
      layer_flags[4] = layer_flags[2];
      // Layer 1.
      layer_flags[5] = layer_flags[3];
      // Layer 0.
      layer_flags[6] = layer_flags[4];
      // Layer 1.
      layer_flags[7] = layer_flags[5];
     break;
    }
    case 9: {
      // 3-layers: Sync points for layer 1 and 2 every 8 frames.
      int ids[4] = {0, 2, 1, 2};
      cfg->ts_periodicity = 4;
      *flag_periodicity = 8;
      cfg->ts_number_layers = 3;
      cfg->ts_rate_decimator[0] = 4;
      cfg->ts_rate_decimator[1] = 2;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF.
      layer_flags[0] = VPX_EFLAG_FORCE_KF  | VP8_EFLAG_NO_REF_GF |
          VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[1] = VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF |
          VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF;
      layer_flags[2] = VP8_EFLAG_NO_REF_GF   | VP8_EFLAG_NO_REF_ARF |
          VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[3] =
      layer_flags[5] = VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF;
      layer_flags[4] = VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF |
          VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
      layer_flags[6] = VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_ARF;
      layer_flags[7] = VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_ENTROPY;
      break;
    }
    case 10: {
      // 3-layers structure where ARF is used as predictor for all frames,
      // and is only updated on key frame.
      // Sync points for layer 1 and 2 every 8 frames.

      int ids[4] = {0, 2, 1, 2};
      cfg->ts_periodicity = 4;
      *flag_periodicity = 8;
      cfg->ts_number_layers = 3;
      cfg->ts_rate_decimator[0] = 4;
      cfg->ts_rate_decimator[1] = 2;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF.
      // Layer 0: predict from L and ARF; update L and G.
      layer_flags[0] = VPX_EFLAG_FORCE_KF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_REF_GF;
      // Layer 2: sync point: predict from L and ARF; update none.
      layer_flags[1] = VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_GF |
          VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST |
          VP8_EFLAG_NO_UPD_ENTROPY;
      // Layer 1: sync point: predict from L and ARF; update G.
      layer_flags[2] = VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_UPD_LAST;
      // Layer 2: predict from L, G, ARF; update none.
      layer_flags[3] = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ENTROPY;
      // Layer 0: predict from L and ARF; update L.
      layer_flags[4] = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_REF_GF;
      // Layer 2: predict from L, G, ARF; update none.
      layer_flags[5] = layer_flags[3];
      // Layer 1: predict from L, G, ARF; update G.
      layer_flags[6] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST;
      // Layer 2: predict from L, G, ARF; update none.
      layer_flags[7] = layer_flags[3];
      break;
    }
    case 11:
    default: {
      // 3-layers structure as in case 10, but no sync/refresh points for
      // layer 1 and 2.
      int ids[4] = {0, 2, 1, 2};
      cfg->ts_periodicity = 4;
      *flag_periodicity = 8;
      cfg->ts_number_layers = 3;
      cfg->ts_rate_decimator[0] = 4;
      cfg->ts_rate_decimator[1] = 2;
      cfg->ts_rate_decimator[2] = 1;
      memcpy(cfg->ts_layer_id, ids, sizeof(ids));
      // 0=L, 1=GF, 2=ARF.
      // Layer 0: predict from L and ARF; update L.
      layer_flags[0] = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_REF_GF;
      layer_flags[4] = layer_flags[0];
      // Layer 1: predict from L, G, ARF; update G.
      layer_flags[2] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST;
      layer_flags[6] = layer_flags[2];
      // Layer 2: predict from L, G, ARF; update none.
      layer_flags[1] = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
          VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ENTROPY;
      layer_flags[3] = layer_flags[1];
      layer_flags[5] = layer_flags[1];
      layer_flags[7] = layer_flags[1];
      break;
    }
  }
}

int main(int argc, char **argv) {
  VpxVideoWriter *outfile[VPX_TS_MAX_LAYERS];
  vpx_codec_ctx_t codec;
  vpx_codec_enc_cfg_t cfg;
  int frame_cnt = 0;
  vpx_image_t raw;
  vpx_codec_err_t res;
  unsigned int width;
  unsigned int height;
  int speed;
  int frame_avail;
  int got_data;
  int flags = 0;
  unsigned int i;
  int pts = 0;  // PTS starts at 0.
  int frame_duration = 1;  // 1 timebase tick per frame.
  int layering_mode = 0;
  int layer_flags[VPX_TS_MAX_PERIODICITY] = {0};
  int flag_periodicity = 1;
  int max_intra_size_pct;
  vpx_svc_layer_id_t layer_id = {0, 0};
  const VpxInterface *encoder = NULL;
  FILE *infile = NULL;
  struct RateControlMetrics rc;
  int64_t cx_time = 0;

  exec_name = argv[0];
  // Check usage and arguments.
  if (argc < 11) {
    die("Usage: %s <infile> <outfile> <codec_type(vp8/vp9)> <width> <height> "
        "<rate_num> <rate_den> <speed> <frame_drop_threshold> <mode> "
        "<Rate_0> ... <Rate_nlayers-1> \n", argv[0]);
  }

  encoder = get_vpx_encoder_by_name(argv[3]);
  if (!encoder)
    die("Unsupported codec.");

  printf("Using %s\n", vpx_codec_iface_name(encoder->interface()));

  width = strtol(argv[4], NULL, 0);
  height = strtol(argv[5], NULL, 0);
  if (width < 16 || width % 2 || height < 16 || height % 2) {
    die("Invalid resolution: %d x %d", width, height);
  }

  layering_mode = strtol(argv[10], NULL, 0);
  if (layering_mode < 0 || layering_mode > 12) {
    die("Invalid layering mode (0..12) %s", argv[10]);
  }

  if (argc != 11 + mode_to_num_layers[layering_mode]) {
    die("Invalid number of arguments");
  }

  if (!vpx_img_alloc(&raw, VPX_IMG_FMT_I420, width, height, 32)) {
    die("Failed to allocate image", width, height);
  }

  // Populate encoder configuration.
  res = vpx_codec_enc_config_default(encoder->interface(), &cfg, 0);
  if (res) {
    printf("Failed to get config: %s\n", vpx_codec_err_to_string(res));
    return EXIT_FAILURE;
  }

  // Update the default configuration with our settings.
  cfg.g_w = width;
  cfg.g_h = height;

  // Timebase format e.g. 30fps: numerator=1, demoninator = 30.
  cfg.g_timebase.num = strtol(argv[6], NULL, 0);
  cfg.g_timebase.den = strtol(argv[7], NULL, 0);

  speed = strtol(argv[8], NULL, 0);
  if (speed < 0) {
    die("Invalid speed setting: must be positive");
  }

  for (i = 11; (int)i < 11 + mode_to_num_layers[layering_mode]; ++i) {
    cfg.ts_target_bitrate[i - 11] = strtol(argv[i], NULL, 0);
  }

  // Real time parameters.
  cfg.rc_dropframe_thresh = strtol(argv[9], NULL, 0);
  cfg.rc_end_usage = VPX_CBR;
  cfg.rc_resize_allowed = 0;
  cfg.rc_min_quantizer = 2;
  cfg.rc_max_quantizer = 56;
  cfg.rc_undershoot_pct = 50;
  cfg.rc_overshoot_pct = 50;
  cfg.rc_buf_initial_sz = 500;
  cfg.rc_buf_optimal_sz = 600;
  cfg.rc_buf_sz = 1000;

  // Enable error resilient mode.
  cfg.g_error_resilient = 1;
  cfg.g_lag_in_frames   = 0;
  cfg.kf_mode = VPX_KF_AUTO;

  // Disable automatic keyframe placement.
  cfg.kf_min_dist = cfg.kf_max_dist = 3000;

  set_temporal_layer_pattern(layering_mode,
                             &cfg,
                             layer_flags,
                             &flag_periodicity);

  set_rate_control_metrics(&rc, &cfg);

  // Target bandwidth for the whole stream.
  // Set to ts_target_bitrate for highest layer (total bitrate).
  cfg.rc_target_bitrate = cfg.ts_target_bitrate[cfg.ts_number_layers - 1];

  // Open input file.
  if (!(infile = fopen(argv[1], "rb"))) {
    die("Failed to open %s for reading", argv[1]);
  }

  // Open an output file for each stream.
  for (i = 0; i < cfg.ts_number_layers; ++i) {
    char file_name[PATH_MAX];
    VpxVideoInfo info;
    info.codec_fourcc = encoder->fourcc;
    info.frame_width = cfg.g_w;
    info.frame_height = cfg.g_h;
    info.time_base.numerator = cfg.g_timebase.num;
    info.time_base.denominator = cfg.g_timebase.den;

    snprintf(file_name, sizeof(file_name), "%s_%d.ivf", argv[2], i);
    outfile[i] = vpx_video_writer_open(file_name, kContainerIVF, &info);
    if (!outfile[i])
      die("Failed to open %s for writing", file_name);
  }
  // No spatial layers in this encoder.
  cfg.ss_number_layers = 1;

  // Initialize codec.
  if (vpx_codec_enc_init(&codec, encoder->interface(), &cfg, 0))
    die_codec(&codec, "Failed to initialize encoder");

  if (strncmp(encoder->name, "vp8", 3) == 0) {
    vpx_codec_control(&codec, VP8E_SET_CPUUSED, -speed);
    vpx_codec_control(&codec, VP8E_SET_NOISE_SENSITIVITY, 1);
  } else if (strncmp(encoder->name, "vp9", 3) == 0) {
      vpx_codec_control(&codec, VP8E_SET_CPUUSED, speed);
      vpx_codec_control(&codec, VP9E_SET_AQ_MODE, 3);
      vpx_codec_control(&codec, VP9E_SET_FRAME_PERIODIC_BOOST, 0);
      vpx_codec_control(&codec, VP8E_SET_NOISE_SENSITIVITY, 0);
      if (vpx_codec_control(&codec, VP9E_SET_SVC, 1)) {
        die_codec(&codec, "Failed to set SVC");
    }
  }
  vpx_codec_control(&codec, VP8E_SET_STATIC_THRESHOLD, 1);
  vpx_codec_control(&codec, VP8E_SET_TOKEN_PARTITIONS, 1);
  // This controls the maximum target size of the key frame.
  // For generating smaller key frames, use a smaller max_intra_size_pct
  // value, like 100 or 200.
  max_intra_size_pct = (int) (((double)cfg.rc_buf_optimal_sz * 0.5)
      * ((double) cfg.g_timebase.den / cfg.g_timebase.num) / 10.0);
  // For low-quality key frame.
  max_intra_size_pct = 200;
  vpx_codec_control(&codec, VP8E_SET_MAX_INTRA_BITRATE_PCT, max_intra_size_pct);

  frame_avail = 1;
  while (frame_avail || got_data) {
    struct vpx_usec_timer timer;
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt;
    // Update the temporal layer_id. No spatial layers in this test.
    layer_id.spatial_layer_id = 0;
    layer_id.temporal_layer_id =
        cfg.ts_layer_id[frame_cnt % cfg.ts_periodicity];
    if (strncmp(encoder->name, "vp9", 3) == 0) {
      vpx_codec_control(&codec, VP9E_SET_SVC_LAYER_ID, &layer_id);
    }
    flags = layer_flags[frame_cnt % flag_periodicity];
    frame_avail = vpx_img_read(&raw, infile);
    if (frame_avail)
      ++rc.layer_input_frames[layer_id.temporal_layer_id];
    vpx_usec_timer_start(&timer);
    if (vpx_codec_encode(&codec, frame_avail? &raw : NULL, pts, 1, flags,
        VPX_DL_REALTIME)) {
      die_codec(&codec, "Failed to encode frame");
    }
    vpx_usec_timer_mark(&timer);
    cx_time += vpx_usec_timer_elapsed(&timer);
    // Reset KF flag.
    if (layering_mode != 7) {
      layer_flags[0] &= ~VPX_EFLAG_FORCE_KF;
    }
    got_data = 0;
    while ( (pkt = vpx_codec_get_cx_data(&codec, &iter)) ) {
      got_data = 1;
      switch (pkt->kind) {
        case VPX_CODEC_CX_FRAME_PKT:
          for (i = cfg.ts_layer_id[frame_cnt % cfg.ts_periodicity];
              i < cfg.ts_number_layers; ++i) {
            vpx_video_writer_write_frame(outfile[i], pkt->data.frame.buf,
                                         pkt->data.frame.sz, pts);
            ++rc.layer_tot_enc_frames[i];
            rc.layer_encoding_bitrate[i] += 8.0 * pkt->data.frame.sz;
            // Keep count of rate control stats per layer (for non-key frames).
            if (i == cfg.ts_layer_id[frame_cnt % cfg.ts_periodicity] &&
                !(pkt->data.frame.flags & VPX_FRAME_IS_KEY)) {
              rc.layer_avg_frame_size[i] += 8.0 * pkt->data.frame.sz;
              rc.layer_avg_rate_mismatch[i] +=
                  fabs(8.0 * pkt->data.frame.sz - rc.layer_pfb[i]) /
                  rc.layer_pfb[i];
              ++rc.layer_enc_frames[i];
            }
          }
          break;
          default:
            break;
      }
    }
    ++frame_cnt;
    pts += frame_duration;
  }
  fclose(infile);
  printout_rate_control_summary(&rc, &cfg, frame_cnt);
  printf("\n");
  printf("Frame cnt and encoding time/FPS stats for encoding: %d %f %f \n",
          frame_cnt,
          1000 * (float)cx_time / (double)(frame_cnt * 1000000),
          1000000 * (double)frame_cnt / (double)cx_time);

  if (vpx_codec_destroy(&codec))
    die_codec(&codec, "Failed to destroy codec");

  // Try to rewrite the output file headers with the actual frame count.
  for (i = 0; i < cfg.ts_number_layers; ++i)
    vpx_video_writer_close(outfile[i]);

  return EXIT_SUCCESS;
}
