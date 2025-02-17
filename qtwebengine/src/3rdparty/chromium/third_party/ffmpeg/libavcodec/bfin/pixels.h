/*
 * BlackFin DSPUTILS COMMON OPTIMIZATIONS HEADER
 *
 * Copyright (C) 2007 Marc Hoffman <mmh@pleasantst.com>
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

#ifndef AVCODEC_BFIN_PIXELS_H
#define AVCODEC_BFIN_PIXELS_H

#include <stdint.h>

#include "libavutil/bfin/attributes.h"

void ff_bfin_z_put_pixels16_xy2(uint8_t *block, const uint8_t *s0,
                                int dest_size, int line_size, int h) attribute_l1_text;
void ff_bfin_z_put_pixels8_xy2(uint8_t *block, const uint8_t *s0,
                               int dest_size, int line_size, int h) attribute_l1_text;

void ff_bfin_put_pixels8uc(uint8_t *block, const uint8_t *s0,
                           const uint8_t *s1, int dest_size, int line_size,
                           int h) attribute_l1_text;
void ff_bfin_put_pixels16uc(uint8_t *block, const uint8_t *s0,
                            const uint8_t *s1, int dest_size, int line_size,
                            int h) attribute_l1_text;

#endif /* AVCODEC_BFIN_PIXELS_H */
