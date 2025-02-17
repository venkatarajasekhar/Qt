/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef R300_TGSI_TO_RC_H
#define R300_TGSI_TO_RC_H

#include "pipe/p_compiler.h"

struct radeon_compiler;

struct tgsi_full_declaration;
struct tgsi_shader_info;
struct tgsi_token;

struct swizzled_imms {
    unsigned index;
    unsigned swizzle;
};

struct tgsi_to_rc {
    struct radeon_compiler * compiler;
    const struct tgsi_shader_info * info;

    int immediate_offset;
    struct swizzled_imms * imms_to_swizzle;
    unsigned imms_to_swizzle_count;

    /* Vertex shaders have no half swizzles, and no way to handle them, so
     * until rc grows proper support, indicate if they're safe to use. */
    boolean use_half_swizzles;

    /* If an error occured. */
    boolean error;
};

void r300_tgsi_to_rc(struct tgsi_to_rc * ttr, const struct tgsi_token * tokens);

#endif /* R300_TGSI_TO_RC_H */
