/*
 * raw FLAC muxer
 * Copyright (c) 2006-2009 Justin Ruggles
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

#include "libavutil/opt.h"
#include "libavcodec/flac.h"
#include "avformat.h"
#include "avio_internal.h"
#include "flacenc.h"
#include "vorbiscomment.h"
#include "libavcodec/bytestream.h"


typedef struct FlacMuxerContext {
    const AVClass *class;
    int write_header;
} FlacMuxerContext;

static int flac_write_block_padding(AVIOContext *pb, unsigned int n_padding_bytes,
                                    int last_block)
{
    avio_w8(pb, last_block ? 0x81 : 0x01);
    avio_wb24(pb, n_padding_bytes);
    ffio_fill(pb, 0, n_padding_bytes);
    return 0;
}

static int flac_write_block_comment(AVIOContext *pb, AVDictionary **m,
                                    int last_block, int bitexact)
{
    const char *vendor = bitexact ? "ffmpeg" : LIBAVFORMAT_IDENT;
    unsigned int len, count;
    uint8_t *p, *p0;

    ff_metadata_conv(m, ff_vorbiscomment_metadata_conv, NULL);

    len = ff_vorbiscomment_length(*m, vendor, &count);
    p0 = av_malloc(len+4);
    if (!p0)
        return AVERROR(ENOMEM);
    p = p0;

    bytestream_put_byte(&p, last_block ? 0x84 : 0x04);
    bytestream_put_be24(&p, len);
    ff_vorbiscomment_write(&p, m, vendor, count);

    avio_write(pb, p0, len+4);
    av_freep(&p0);
    p = NULL;

    return 0;
}

static int flac_write_header(struct AVFormatContext *s)
{
    int ret;
    int padding = s->metadata_header_padding;
    AVCodecContext *codec = s->streams[0]->codec;
    FlacMuxerContext *c   = s->priv_data;

    if (!c->write_header)
        return 0;

    if (s->nb_streams > 1) {
        av_log(s, AV_LOG_ERROR, "only one stream is supported\n");
        return AVERROR(EINVAL);
    }
    if (codec->codec_id != AV_CODEC_ID_FLAC) {
        av_log(s, AV_LOG_ERROR, "unsupported codec\n");
        return AVERROR(EINVAL);
    }

    if (padding < 0)
        padding = 8192;
    /* The FLAC specification states that 24 bits are used to represent the
     * size of a metadata block so we must clip this value to 2^24-1. */
    padding = av_clip_c(padding, 0, 16777215);

    ret = ff_flac_write_header(s->pb, codec, 0);
    if (ret)
        return ret;

    ret = flac_write_block_comment(s->pb, &s->metadata, !padding,
                                   s->flags & AVFMT_FLAG_BITEXACT);
    if (ret)
        return ret;

    /* The command line flac encoder defaults to placing a seekpoint
     * every 10s.  So one might add padding to allow that later
     * but there seems to be no simple way to get the duration here.
     * So just add the amount requested by the user. */
    if (padding)
        flac_write_block_padding(s->pb, padding, 1);

    return ret;
}

static int flac_write_trailer(struct AVFormatContext *s)
{
    AVIOContext *pb = s->pb;
    uint8_t *streaminfo;
    enum FLACExtradataFormat format;
    int64_t file_size;
    FlacMuxerContext *c = s->priv_data;

    if (!c->write_header)
        return 0;

    if (!avpriv_flac_is_extradata_valid(s->streams[0]->codec, &format, &streaminfo))
        return -1;

    if (pb->seekable) {
        /* rewrite the STREAMINFO header block data */
        file_size = avio_tell(pb);
        avio_seek(pb, 8, SEEK_SET);
        avio_write(pb, streaminfo, FLAC_STREAMINFO_SIZE);
        avio_seek(pb, file_size, SEEK_SET);
        avio_flush(pb);
    } else {
        av_log(s, AV_LOG_WARNING, "unable to rewrite FLAC header.\n");
    }
    return 0;
}

static int flac_write_packet(struct AVFormatContext *s, AVPacket *pkt)
{
    avio_write(s->pb, pkt->data, pkt->size);
    return 0;
}

static const AVOption flacenc_options[] = {
    { "write_header", "Write the file header", offsetof(FlacMuxerContext, write_header), AV_OPT_TYPE_INT, {.i64 = 1}, 0, 1, AV_OPT_FLAG_ENCODING_PARAM },
    { NULL },
};

static const AVClass flac_muxer_class = {
    .class_name = "flac muxer",
    .item_name  = av_default_item_name,
    .option     = flacenc_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVOutputFormat ff_flac_muxer = {
    .name              = "flac",
    .long_name         = NULL_IF_CONFIG_SMALL("raw FLAC"),
    .priv_data_size    = sizeof(FlacMuxerContext),
    .mime_type         = "audio/x-flac",
    .extensions        = "flac",
    .audio_codec       = AV_CODEC_ID_FLAC,
    .video_codec       = AV_CODEC_ID_NONE,
    .write_header      = flac_write_header,
    .write_packet      = flac_write_packet,
    .write_trailer     = flac_write_trailer,
    .flags             = AVFMT_NOTIMESTAMPS,
    .priv_class        = &flac_muxer_class,
};
