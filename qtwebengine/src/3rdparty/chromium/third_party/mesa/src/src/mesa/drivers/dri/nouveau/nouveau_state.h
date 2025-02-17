/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NOUVEAU_STATE_H__
#define __NOUVEAU_STATE_H__

enum {
	NOUVEAU_STATE_ALPHA_FUNC,
	NOUVEAU_STATE_BLEND_COLOR,
	NOUVEAU_STATE_BLEND_EQUATION,
	NOUVEAU_STATE_BLEND_FUNC,
	NOUVEAU_STATE_CLIP_PLANE0,
	NOUVEAU_STATE_CLIP_PLANE1,
	NOUVEAU_STATE_CLIP_PLANE2,
	NOUVEAU_STATE_CLIP_PLANE3,
	NOUVEAU_STATE_CLIP_PLANE4,
	NOUVEAU_STATE_CLIP_PLANE5,
	NOUVEAU_STATE_COLOR_MASK,
	NOUVEAU_STATE_COLOR_MATERIAL,
	NOUVEAU_STATE_CULL_FACE,
	NOUVEAU_STATE_FRONT_FACE,
	NOUVEAU_STATE_DEPTH,
	NOUVEAU_STATE_DITHER,
	NOUVEAU_STATE_FRAG,
	NOUVEAU_STATE_FRAMEBUFFER,
	NOUVEAU_STATE_FOG,
	NOUVEAU_STATE_LIGHT_ENABLE,
	NOUVEAU_STATE_LIGHT_MODEL,
	NOUVEAU_STATE_LIGHT_SOURCE0,
	NOUVEAU_STATE_LIGHT_SOURCE1,
	NOUVEAU_STATE_LIGHT_SOURCE2,
	NOUVEAU_STATE_LIGHT_SOURCE3,
	NOUVEAU_STATE_LIGHT_SOURCE4,
	NOUVEAU_STATE_LIGHT_SOURCE5,
	NOUVEAU_STATE_LIGHT_SOURCE6,
	NOUVEAU_STATE_LIGHT_SOURCE7,
	NOUVEAU_STATE_LINE_STIPPLE,
	NOUVEAU_STATE_LINE_MODE,
	NOUVEAU_STATE_LOGIC_OPCODE,
	NOUVEAU_STATE_MATERIAL_FRONT_AMBIENT,
	NOUVEAU_STATE_MATERIAL_BACK_AMBIENT,
	NOUVEAU_STATE_MATERIAL_FRONT_DIFFUSE,
	NOUVEAU_STATE_MATERIAL_BACK_DIFFUSE,
	NOUVEAU_STATE_MATERIAL_FRONT_SPECULAR,
	NOUVEAU_STATE_MATERIAL_BACK_SPECULAR,
	NOUVEAU_STATE_MATERIAL_FRONT_SHININESS,
	NOUVEAU_STATE_MATERIAL_BACK_SHININESS,
	NOUVEAU_STATE_MODELVIEW,
	NOUVEAU_STATE_POINT_MODE,
	NOUVEAU_STATE_POINT_PARAMETER,
	NOUVEAU_STATE_POLYGON_MODE,
	NOUVEAU_STATE_POLYGON_OFFSET,
	NOUVEAU_STATE_POLYGON_STIPPLE,
	NOUVEAU_STATE_PROJECTION,
	NOUVEAU_STATE_RENDER_MODE,
	NOUVEAU_STATE_SCISSOR,
	NOUVEAU_STATE_SHADE_MODEL,
	NOUVEAU_STATE_STENCIL_FUNC,
	NOUVEAU_STATE_STENCIL_MASK,
	NOUVEAU_STATE_STENCIL_OP,
	NOUVEAU_STATE_TEX_ENV0,
	NOUVEAU_STATE_TEX_ENV1,
	NOUVEAU_STATE_TEX_ENV2,
	NOUVEAU_STATE_TEX_ENV3,
	NOUVEAU_STATE_TEX_GEN0,
	NOUVEAU_STATE_TEX_GEN1,
	NOUVEAU_STATE_TEX_GEN2,
	NOUVEAU_STATE_TEX_GEN3,
	NOUVEAU_STATE_TEX_MAT0,
	NOUVEAU_STATE_TEX_MAT1,
	NOUVEAU_STATE_TEX_MAT2,
	NOUVEAU_STATE_TEX_MAT3,
	NOUVEAU_STATE_TEX_OBJ0,
	NOUVEAU_STATE_TEX_OBJ1,
	NOUVEAU_STATE_TEX_OBJ2,
	NOUVEAU_STATE_TEX_OBJ3,
	NOUVEAU_STATE_VIEWPORT,
	NUM_NOUVEAU_STATE,

	/* Room for card-specific states. */

	MAX_NOUVEAU_STATE = NUM_NOUVEAU_STATE + 16,
};

typedef void (*nouveau_state_func)(struct gl_context *ctx, int emit);

void
nouveau_state_init(struct gl_context *ctx);

void
nouveau_emit_nothing(struct gl_context *ctx, int emit);

int
nouveau_next_dirty_state(struct gl_context *ctx);

void
nouveau_state_emit(struct gl_context *ctx);

#endif
