/*
 * MJPEG encoder
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2003 Alex Beregszaszi
 * Copyright (c) 2003-2004 Michael Niedermayer
 *
 * Support for external huffman table, various fixes (AVID workaround),
 * aspecting, new decode_frame mechanism and apple mjpeg-b support
 *                                  by Alex Beregszaszi
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
 * MJPEG encoder.
 */

#include "libavutil/pixdesc.h"

#include "avcodec.h"
#include "mpegvideo.h"
#include "mjpeg.h"
#include "mjpegenc.h"

av_cold int ff_mjpeg_encode_init(MpegEncContext *s)
{
    MJpegContext *m;

    if (s->width > 65500 || s->height > 65500) {
        av_log(s, AV_LOG_ERROR, "JPEG does not support resolutions above 65500x65500\n");
        return AVERROR(EINVAL);
    }

    m = av_malloc(sizeof(MJpegContext));
    if (!m)
        return AVERROR(ENOMEM);

    s->min_qcoeff=-1023;
    s->max_qcoeff= 1023;

    /* build all the huffman tables */
    ff_mjpeg_build_huffman_codes(m->huff_size_dc_luminance,
                                 m->huff_code_dc_luminance,
                                 avpriv_mjpeg_bits_dc_luminance,
                                 avpriv_mjpeg_val_dc);
    ff_mjpeg_build_huffman_codes(m->huff_size_dc_chrominance,
                                 m->huff_code_dc_chrominance,
                                 avpriv_mjpeg_bits_dc_chrominance,
                                 avpriv_mjpeg_val_dc);
    ff_mjpeg_build_huffman_codes(m->huff_size_ac_luminance,
                                 m->huff_code_ac_luminance,
                                 avpriv_mjpeg_bits_ac_luminance,
                                 avpriv_mjpeg_val_ac_luminance);
    ff_mjpeg_build_huffman_codes(m->huff_size_ac_chrominance,
                                 m->huff_code_ac_chrominance,
                                 avpriv_mjpeg_bits_ac_chrominance,
                                 avpriv_mjpeg_val_ac_chrominance);

    s->mjpeg_ctx = m;
    return 0;
}

void ff_mjpeg_encode_close(MpegEncContext *s)
{
    av_free(s->mjpeg_ctx);
}

/* table_class: 0 = DC coef, 1 = AC coefs */
static int put_huffman_table(PutBitContext *p, int table_class, int table_id,
                             const uint8_t *bits_table, const uint8_t *value_table)
{
    int n, i;

    put_bits(p, 4, table_class);
    put_bits(p, 4, table_id);

    n = 0;
    for(i=1;i<=16;i++) {
        n += bits_table[i];
        put_bits(p, 8, bits_table[i]);
    }

    for(i=0;i<n;i++)
        put_bits(p, 8, value_table[i]);

    return n + 17;
}

static void jpeg_table_header(AVCodecContext *avctx, PutBitContext *p,
                              ScanTable *intra_scantable,
                              uint16_t luma_intra_matrix[64],
                              uint16_t chroma_intra_matrix[64],
                              int hsample[3])
{
    int i, j, size;
    uint8_t *ptr;

    if (avctx->codec_id != AV_CODEC_ID_LJPEG) {
        int matrix_count = 1 + !!memcmp(luma_intra_matrix,
                                        chroma_intra_matrix,
                                        sizeof(luma_intra_matrix[0]) * 64);
    /* quant matrixes */
    put_marker(p, DQT);
    put_bits(p, 16, 2 + matrix_count * (1 + 64));
    put_bits(p, 4, 0); /* 8 bit precision */
    put_bits(p, 4, 0); /* table 0 */
    for(i=0;i<64;i++) {
        j = intra_scantable->permutated[i];
        put_bits(p, 8, luma_intra_matrix[j]);
    }

        if (matrix_count > 1) {
            put_bits(p, 4, 0); /* 8 bit precision */
            put_bits(p, 4, 1); /* table 1 */
            for(i=0;i<64;i++) {
                j = intra_scantable->permutated[i];
                put_bits(p, 8, chroma_intra_matrix[j]);
            }
        }
    }

    if(avctx->active_thread_type & FF_THREAD_SLICE){
        put_marker(p, DRI);
        put_bits(p, 16, 4);
        put_bits(p, 16, (avctx->width-1)/(8*hsample[0]) + 1);
    }

    /* huffman table */
    put_marker(p, DHT);
    flush_put_bits(p);
    ptr = put_bits_ptr(p);
    put_bits(p, 16, 0); /* patched later */
    size = 2;
    size += put_huffman_table(p, 0, 0, avpriv_mjpeg_bits_dc_luminance,
                              avpriv_mjpeg_val_dc);
    size += put_huffman_table(p, 0, 1, avpriv_mjpeg_bits_dc_chrominance,
                              avpriv_mjpeg_val_dc);

    size += put_huffman_table(p, 1, 0, avpriv_mjpeg_bits_ac_luminance,
                              avpriv_mjpeg_val_ac_luminance);
    size += put_huffman_table(p, 1, 1, avpriv_mjpeg_bits_ac_chrominance,
                              avpriv_mjpeg_val_ac_chrominance);
    AV_WB16(ptr, size);
}

static void jpeg_put_comments(AVCodecContext *avctx, PutBitContext *p)
{
    int size;
    uint8_t *ptr;

    if (avctx->sample_aspect_ratio.num > 0 && avctx->sample_aspect_ratio.den > 0) {
        /* JFIF header */
        put_marker(p, APP0);
        put_bits(p, 16, 16);
        avpriv_put_string(p, "JFIF", 1); /* this puts the trailing zero-byte too */
        put_bits(p, 16, 0x0102);         /* v 1.02 */
        put_bits(p,  8, 0);              /* units type: 0 - aspect ratio */
        put_bits(p, 16, avctx->sample_aspect_ratio.num);
        put_bits(p, 16, avctx->sample_aspect_ratio.den);
        put_bits(p, 8, 0); /* thumbnail width */
        put_bits(p, 8, 0); /* thumbnail height */
    }

    /* comment */
    if (!(avctx->flags & CODEC_FLAG_BITEXACT)) {
        put_marker(p, COM);
        flush_put_bits(p);
        ptr = put_bits_ptr(p);
        put_bits(p, 16, 0); /* patched later */
        avpriv_put_string(p, LIBAVCODEC_IDENT, 1);
        size = strlen(LIBAVCODEC_IDENT)+3;
        AV_WB16(ptr, size);
    }

    if (avctx->pix_fmt == AV_PIX_FMT_YUV420P ||
        avctx->pix_fmt == AV_PIX_FMT_YUV422P ||
        avctx->pix_fmt == AV_PIX_FMT_YUV444P) {
        put_marker(p, COM);
        flush_put_bits(p);
        ptr = put_bits_ptr(p);
        put_bits(p, 16, 0); /* patched later */
        avpriv_put_string(p, "CS=ITU601", 1);
        size = strlen("CS=ITU601")+3;
        AV_WB16(ptr, size);
    }
}

void ff_mjpeg_init_hvsample(AVCodecContext *avctx, int hsample[3], int vsample[3])
{
    int chroma_h_shift, chroma_v_shift;

    av_pix_fmt_get_chroma_sub_sample(avctx->pix_fmt, &chroma_h_shift,
                                     &chroma_v_shift);
    if (avctx->codec->id == AV_CODEC_ID_LJPEG &&
        (   avctx->pix_fmt == AV_PIX_FMT_BGR0
         || avctx->pix_fmt == AV_PIX_FMT_BGRA
         || avctx->pix_fmt == AV_PIX_FMT_BGR24)) {
        vsample[0] = hsample[0] =
        vsample[1] = hsample[1] =
        vsample[2] = hsample[2] = 1;
    } else if (avctx->pix_fmt == AV_PIX_FMT_YUV444P || avctx->pix_fmt == AV_PIX_FMT_YUVJ444P) {
        vsample[0] = vsample[1] = vsample[2] = 2;
        hsample[0] = hsample[1] = hsample[2] = 1;
    } else {
        vsample[0] = 2;
        vsample[1] = 2 >> chroma_v_shift;
        vsample[2] = 2 >> chroma_v_shift;
        hsample[0] = 2;
        hsample[1] = 2 >> chroma_h_shift;
        hsample[2] = 2 >> chroma_h_shift;
    }
}

void ff_mjpeg_encode_picture_header(AVCodecContext *avctx, PutBitContext *pb,
                                    ScanTable *intra_scantable,
                                    uint16_t luma_intra_matrix[64],
                                    uint16_t chroma_intra_matrix[64])
{
    const int lossless = avctx->codec_id != AV_CODEC_ID_MJPEG && avctx->codec_id != AV_CODEC_ID_AMV;
    int hsample[3], vsample[3];
    int i;
    int chroma_matrix = !!memcmp(luma_intra_matrix,
                                 chroma_intra_matrix,
                                 sizeof(luma_intra_matrix[0])*64);

    ff_mjpeg_init_hvsample(avctx, hsample, vsample);

    put_marker(pb, SOI);

    // hack for AMV mjpeg format
    if(avctx->codec_id == AV_CODEC_ID_AMV) goto end;

    jpeg_put_comments(avctx, pb);

    jpeg_table_header(avctx, pb, intra_scantable, luma_intra_matrix, chroma_intra_matrix, hsample);

    switch (avctx->codec_id) {
    case AV_CODEC_ID_MJPEG:  put_marker(pb, SOF0 ); break;
    case AV_CODEC_ID_LJPEG:  put_marker(pb, SOF3 ); break;
    default: av_assert0(0);
    }

    put_bits(pb, 16, 17);
    if (lossless && (  avctx->pix_fmt == AV_PIX_FMT_BGR0
                    || avctx->pix_fmt == AV_PIX_FMT_BGRA
                    || avctx->pix_fmt == AV_PIX_FMT_BGR24))
        put_bits(pb, 8, 9); /* 9 bits/component RCT */
    else
        put_bits(pb, 8, 8); /* 8 bits/component */
    put_bits(pb, 16, avctx->height);
    put_bits(pb, 16, avctx->width);
    put_bits(pb, 8, 3); /* 3 components */

    /* Y component */
    put_bits(pb, 8, 1); /* component number */
    put_bits(pb, 4, hsample[0]); /* H factor */
    put_bits(pb, 4, vsample[0]); /* V factor */
    put_bits(pb, 8, 0); /* select matrix */

    /* Cb component */
    put_bits(pb, 8, 2); /* component number */
    put_bits(pb, 4, hsample[1]); /* H factor */
    put_bits(pb, 4, vsample[1]); /* V factor */
    put_bits(pb, 8, lossless ? 0 : chroma_matrix); /* select matrix */

    /* Cr component */
    put_bits(pb, 8, 3); /* component number */
    put_bits(pb, 4, hsample[2]); /* H factor */
    put_bits(pb, 4, vsample[2]); /* V factor */
    put_bits(pb, 8, lossless ? 0 : chroma_matrix); /* select matrix */

    /* scan header */
    put_marker(pb, SOS);
    put_bits(pb, 16, 12); /* length */
    put_bits(pb, 8, 3); /* 3 components */

    /* Y component */
    put_bits(pb, 8, 1); /* index */
    put_bits(pb, 4, 0); /* DC huffman table index */
    put_bits(pb, 4, 0); /* AC huffman table index */

    /* Cb component */
    put_bits(pb, 8, 2); /* index */
    put_bits(pb, 4, 1); /* DC huffman table index */
    put_bits(pb, 4, lossless ? 0 : 1); /* AC huffman table index */

    /* Cr component */
    put_bits(pb, 8, 3); /* index */
    put_bits(pb, 4, 1); /* DC huffman table index */
    put_bits(pb, 4, lossless ? 0 : 1); /* AC huffman table index */

    put_bits(pb, 8, lossless ? avctx->prediction_method + 1 : 0); /* Ss (not used) */

    switch (avctx->codec_id) {
    case AV_CODEC_ID_MJPEG:  put_bits(pb, 8, 63); break; /* Se (not used) */
    case AV_CODEC_ID_LJPEG:  put_bits(pb, 8,  0); break; /* not used */
    default: av_assert0(0);
    }

    put_bits(pb, 8, 0); /* Ah/Al (not used) */

end:
    if (!lossless) {
        MpegEncContext *s = avctx->priv_data;
        av_assert0(avctx->codec->priv_data_size == sizeof(MpegEncContext));

        s->esc_pos = put_bits_count(pb) >> 3;
        for(i=1; i<s->slice_context_count; i++)
            s->thread_context[i]->esc_pos = 0;
    }
}

void ff_mjpeg_escape_FF(PutBitContext *pb, int start)
{
    int size;
    int i, ff_count;
    uint8_t *buf = pb->buf + start;
    int align= (-(size_t)(buf))&3;
    int pad = (-put_bits_count(pb))&7;

    if (pad)
        put_bits(pb, pad, (1<<pad)-1);

    flush_put_bits(pb);
    size = put_bits_count(pb) - start * 8;

    av_assert1((size&7) == 0);
    size >>= 3;

    ff_count=0;
    for(i=0; i<size && i<align; i++){
        if(buf[i]==0xFF) ff_count++;
    }
    for(; i<size-15; i+=16){
        int acc, v;

        v= *(uint32_t*)(&buf[i]);
        acc= (((v & (v>>4))&0x0F0F0F0F)+0x01010101)&0x10101010;
        v= *(uint32_t*)(&buf[i+4]);
        acc+=(((v & (v>>4))&0x0F0F0F0F)+0x01010101)&0x10101010;
        v= *(uint32_t*)(&buf[i+8]);
        acc+=(((v & (v>>4))&0x0F0F0F0F)+0x01010101)&0x10101010;
        v= *(uint32_t*)(&buf[i+12]);
        acc+=(((v & (v>>4))&0x0F0F0F0F)+0x01010101)&0x10101010;

        acc>>=4;
        acc+= (acc>>16);
        acc+= (acc>>8);
        ff_count+= acc&0xFF;
    }
    for(; i<size; i++){
        if(buf[i]==0xFF) ff_count++;
    }

    if(ff_count==0) return;

    flush_put_bits(pb);
    skip_put_bytes(pb, ff_count);

    for(i=size-1; ff_count; i--){
        int v= buf[i];

        if(v==0xFF){
            buf[i+ff_count]= 0;
            ff_count--;
        }

        buf[i+ff_count]= v;
    }
}

void ff_mjpeg_encode_stuffing(MpegEncContext *s)
{
    int i;
    PutBitContext *pbc = &s->pb;
    int mb_y = s->mb_y - !s->mb_x;

    ff_mjpeg_escape_FF(pbc, s->esc_pos);

    if((s->avctx->active_thread_type & FF_THREAD_SLICE) && mb_y < s->mb_height)
        put_marker(pbc, RST0 + (mb_y&7));
    s->esc_pos = put_bits_count(pbc) >> 3;

    for(i=0; i<3; i++)
        s->last_dc[i] = 128 << s->intra_dc_precision;
}

void ff_mjpeg_encode_picture_trailer(PutBitContext *pb, int header_bits)
{
    av_assert1((header_bits & 7) == 0);

    put_marker(pb, EOI);
}

void ff_mjpeg_encode_dc(PutBitContext *pb, int val,
                        uint8_t *huff_size, uint16_t *huff_code)
{
    int mant, nbits;

    if (val == 0) {
        put_bits(pb, huff_size[0], huff_code[0]);
    } else {
        mant = val;
        if (val < 0) {
            val = -val;
            mant--;
        }

        nbits= av_log2_16bit(val) + 1;

        put_bits(pb, huff_size[nbits], huff_code[nbits]);

        put_sbits(pb, nbits, mant);
    }
}

static void encode_block(MpegEncContext *s, int16_t *block, int n)
{
    int mant, nbits, code, i, j;
    int component, dc, run, last_index, val;
    MJpegContext *m = s->mjpeg_ctx;
    uint8_t *huff_size_ac;
    uint16_t *huff_code_ac;

    /* DC coef */
    component = (n <= 3 ? 0 : (n&1) + 1);
    dc = block[0]; /* overflow is impossible */
    val = dc - s->last_dc[component];
    if (n < 4) {
        ff_mjpeg_encode_dc(&s->pb, val, m->huff_size_dc_luminance, m->huff_code_dc_luminance);
        huff_size_ac = m->huff_size_ac_luminance;
        huff_code_ac = m->huff_code_ac_luminance;
    } else {
        ff_mjpeg_encode_dc(&s->pb, val, m->huff_size_dc_chrominance, m->huff_code_dc_chrominance);
        huff_size_ac = m->huff_size_ac_chrominance;
        huff_code_ac = m->huff_code_ac_chrominance;
    }
    s->last_dc[component] = dc;

    /* AC coefs */

    run = 0;
    last_index = s->block_last_index[n];
    for(i=1;i<=last_index;i++) {
        j = s->intra_scantable.permutated[i];
        val = block[j];
        if (val == 0) {
            run++;
        } else {
            while (run >= 16) {
                put_bits(&s->pb, huff_size_ac[0xf0], huff_code_ac[0xf0]);
                run -= 16;
            }
            mant = val;
            if (val < 0) {
                val = -val;
                mant--;
            }

            nbits= av_log2_16bit(val) + 1;
            code = (run << 4) | nbits;

            put_bits(&s->pb, huff_size_ac[code], huff_code_ac[code]);

            put_sbits(&s->pb, nbits, mant);
            run = 0;
        }
    }

    /* output EOB only if not already 64 values */
    if (last_index < 63 || run != 0)
        put_bits(&s->pb, huff_size_ac[0], huff_code_ac[0]);
}

void ff_mjpeg_encode_mb(MpegEncContext *s, int16_t block[12][64])
{
    int i;
    if (s->chroma_format == CHROMA_444) {
        encode_block(s, block[0], 0);
        encode_block(s, block[2], 2);
        encode_block(s, block[4], 4);
        encode_block(s, block[8], 8);
        encode_block(s, block[5], 5);
        encode_block(s, block[9], 9);

        if (16*s->mb_x+8 < s->width) {
            encode_block(s, block[1], 1);
            encode_block(s, block[3], 3);
            encode_block(s, block[6], 6);
            encode_block(s, block[10], 10);
            encode_block(s, block[7], 7);
            encode_block(s, block[11], 11);
        }
    } else {
        for(i=0;i<5;i++) {
            encode_block(s, block[i], i);
        }
        if (s->chroma_format == CHROMA_420) {
            encode_block(s, block[5], 5);
        } else {
            encode_block(s, block[6], 6);
            encode_block(s, block[5], 5);
            encode_block(s, block[7], 7);
        }
    }

    s->i_tex_bits += get_bits_diff(s);
}

// maximum over s->mjpeg_vsample[i]
#define V_MAX 2
static int amv_encode_picture(AVCodecContext *avctx, AVPacket *pkt,
                              const AVFrame *pic_arg, int *got_packet)

{
    MpegEncContext *s = avctx->priv_data;
    AVFrame *pic;
    int i, ret;
    int chroma_h_shift, chroma_v_shift;

    av_pix_fmt_get_chroma_sub_sample(avctx->pix_fmt, &chroma_h_shift, &chroma_v_shift);

    //CODEC_FLAG_EMU_EDGE have to be cleared
    if(s->avctx->flags & CODEC_FLAG_EMU_EDGE)
        return AVERROR(EINVAL);

    pic = av_frame_clone(pic_arg);
    if (!pic)
        return AVERROR(ENOMEM);
    //picture should be flipped upside-down
    for(i=0; i < 3; i++) {
        int vsample = i ? 2 >> chroma_v_shift : 2;
        pic->data[i] += (pic->linesize[i] * (vsample * (8 * s->mb_height -((s->height/V_MAX)&7)) - 1 ));
        pic->linesize[i] *= -1;
    }
    ret = ff_MPV_encode_picture(avctx, pkt, pic, got_packet);
    av_frame_free(&pic);
    return ret;
}

#if CONFIG_MJPEG_ENCODER
AVCodec ff_mjpeg_encoder = {
    .name           = "mjpeg",
    .long_name      = NULL_IF_CONFIG_SMALL("MJPEG (Motion JPEG)"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MJPEG,
    .priv_data_size = sizeof(MpegEncContext),
    .init           = ff_MPV_encode_init,
    .encode2        = ff_MPV_encode_picture,
    .close          = ff_MPV_encode_end,
    .capabilities   = CODEC_CAP_SLICE_THREADS | CODEC_CAP_FRAME_THREADS | CODEC_CAP_INTRA_ONLY,
    .pix_fmts       = (const enum AVPixelFormat[]){
        AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUVJ444P, AV_PIX_FMT_NONE
    },
};
#endif
#if CONFIG_AMV_ENCODER
AVCodec ff_amv_encoder = {
    .name           = "amv",
    .long_name      = NULL_IF_CONFIG_SMALL("AMV Video"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_AMV,
    .priv_data_size = sizeof(MpegEncContext),
    .init           = ff_MPV_encode_init,
    .encode2        = amv_encode_picture,
    .close          = ff_MPV_encode_end,
    .pix_fmts       = (const enum AVPixelFormat[]){
        AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_NONE
    },
};
#endif
