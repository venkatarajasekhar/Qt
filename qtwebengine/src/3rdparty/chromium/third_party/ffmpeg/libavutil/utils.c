/*
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

#include "config.h"
#include "avutil.h"
#include "avassert.h"
#include "samplefmt.h"
#include "pixdesc.h"

/**
 * @file
 * various utility functions
 */

unsigned avutil_version(void)
{
    static int checks_done;
    if (checks_done)
        return LIBAVUTIL_VERSION_INT;

    av_assert0(AV_PIX_FMT_VDA_VLD == 81); //check if the pix fmt enum has not had anything inserted or removed by mistake
    av_assert0(AV_SAMPLE_FMT_DBLP == 9);
    av_assert0(AVMEDIA_TYPE_ATTACHMENT == 4);
    av_assert0(AV_PICTURE_TYPE_BI == 7);
    av_assert0(LIBAVUTIL_VERSION_MICRO >= 100);
    av_assert0(HAVE_MMX2 == HAVE_MMXEXT);

    av_assert0(((size_t)-1) > 0); // C gurantees this but if false on a platform we care about revert at least b284e1ffe343d6697fb950d1ee517bafda8a9844

    if (av_sat_dadd32(1, 2) != 5) {
        av_log(NULL, AV_LOG_FATAL, "Libavutil has been build with a broken binutils, please upgrade binutils and rebuild\n");
        abort();
    }

    if (llrint(1LL<<60) != 1LL<<60) {
        av_log(NULL, AV_LOG_ERROR, "Libavutil has been linked to a broken llrint()\n");
    }

#if defined(ASSERT_LEVEL) && ASSERT_LEVEL > 0
    ff_check_pixfmt_descriptors();
#endif
    checks_done = 1;
    return LIBAVUTIL_VERSION_INT;
}

const char *avutil_configuration(void)
{
    return FFMPEG_CONFIGURATION;
}

const char *avutil_license(void)
{
#define LICENSE_PREFIX "libavutil license: "
    return LICENSE_PREFIX FFMPEG_LICENSE + sizeof(LICENSE_PREFIX) - 1;
}

const char *av_get_media_type_string(enum AVMediaType media_type)
{
    switch (media_type) {
    case AVMEDIA_TYPE_VIDEO:      return "video";
    case AVMEDIA_TYPE_AUDIO:      return "audio";
    case AVMEDIA_TYPE_DATA:       return "data";
    case AVMEDIA_TYPE_SUBTITLE:   return "subtitle";
    case AVMEDIA_TYPE_ATTACHMENT: return "attachment";
    default:                      return NULL;
    }
}

char av_get_picture_type_char(enum AVPictureType pict_type)
{
    switch (pict_type) {
    case AV_PICTURE_TYPE_I:  return 'I';
    case AV_PICTURE_TYPE_P:  return 'P';
    case AV_PICTURE_TYPE_B:  return 'B';
    case AV_PICTURE_TYPE_S:  return 'S';
    case AV_PICTURE_TYPE_SI: return 'i';
    case AV_PICTURE_TYPE_SP: return 'p';
    case AV_PICTURE_TYPE_BI: return 'b';
    default:                 return '?';
    }
}

unsigned av_int_list_length_for_size(unsigned elsize,
                                     const void *list, uint64_t term)
{
    unsigned i;

    if (!list)
        return 0;
#define LIST_LENGTH(type) \
    { type t = term, *l = (type *)list; for (i = 0; l[i] != t; i++); }
    switch (elsize) {
    case 1: LIST_LENGTH(uint8_t);  break;
    case 2: LIST_LENGTH(uint16_t); break;
    case 4: LIST_LENGTH(uint32_t); break;
    case 8: LIST_LENGTH(uint64_t); break;
    default: av_assert0(!"valid element size");
    }
    return i;
}

AVRational av_get_time_base_q(void)
{
    return (AVRational){1, AV_TIME_BASE};
}
