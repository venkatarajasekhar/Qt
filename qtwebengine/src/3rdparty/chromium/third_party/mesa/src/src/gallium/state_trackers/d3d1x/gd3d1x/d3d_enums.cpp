/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
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
 **************************************************************************/

#include "d3d1x_private.h"

unsigned d3d11_to_pipe_blend[D3D11_BLEND_COUNT] =
{
	PIPE_BLENDFACTOR_ONE, /* absent in D3D11, but apparently accepted */
	PIPE_BLENDFACTOR_ZERO,
	PIPE_BLENDFACTOR_ONE,
	PIPE_BLENDFACTOR_SRC_COLOR,
	PIPE_BLENDFACTOR_INV_SRC_COLOR,
	PIPE_BLENDFACTOR_SRC_ALPHA,
	PIPE_BLENDFACTOR_INV_SRC_ALPHA,
	PIPE_BLENDFACTOR_DST_ALPHA,
	PIPE_BLENDFACTOR_INV_DST_ALPHA,
	PIPE_BLENDFACTOR_DST_COLOR,
	PIPE_BLENDFACTOR_INV_DST_COLOR,
	PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE,
	0, /* absent in D3D11 */
	0, /* absent in D3D11 */
	PIPE_BLENDFACTOR_CONST_COLOR,
	PIPE_BLENDFACTOR_INV_CONST_COLOR,
	PIPE_BLENDFACTOR_SRC1_COLOR,
	PIPE_BLENDFACTOR_INV_SRC1_COLOR,
	PIPE_BLENDFACTOR_SRC1_ALPHA,
	PIPE_BLENDFACTOR_INV_SRC1_ALPHA
};

unsigned d3d11_to_pipe_usage[D3D11_USAGE_COUNT] =
{
	PIPE_USAGE_DEFAULT,
	PIPE_USAGE_IMMUTABLE,
	PIPE_USAGE_DYNAMIC,
	PIPE_USAGE_STAGING
};

unsigned d3d11_to_pipe_stencil_op[D3D11_STENCIL_OP_COUNT] =
{
	PIPE_STENCIL_OP_KEEP,
	PIPE_STENCIL_OP_KEEP,
	PIPE_STENCIL_OP_ZERO,
	PIPE_STENCIL_OP_REPLACE,
	PIPE_STENCIL_OP_INCR,
	PIPE_STENCIL_OP_DECR,
	PIPE_STENCIL_OP_INVERT,
	PIPE_STENCIL_OP_INCR_WRAP,
	PIPE_STENCIL_OP_DECR_WRAP,
};

unsigned d3d11_to_pipe_wrap[D3D11_TEXTURE_ADDRESS_COUNT] =
{
	PIPE_TEX_WRAP_REPEAT,
	PIPE_TEX_WRAP_REPEAT,
	PIPE_TEX_WRAP_MIRROR_REPEAT,
	PIPE_TEX_WRAP_CLAMP_TO_EDGE,
	PIPE_TEX_WRAP_CLAMP_TO_BORDER,
	PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE,
};

unsigned d3d11_to_pipe_query[D3D11_QUERY_COUNT] =
{
	PIPE_QUERY_GPU_FINISHED,
	PIPE_QUERY_OCCLUSION_COUNTER,
	PIPE_QUERY_TIMESTAMP,
	PIPE_QUERY_TIMESTAMP_DISJOINT,
	PIPE_QUERY_PIPELINE_STATISTICS,
	PIPE_QUERY_OCCLUSION_PREDICATE,
	PIPE_QUERY_SO_STATISTICS,
	PIPE_QUERY_SO_OVERFLOW_PREDICATE,
	/* per-stream SO queries */
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
	PIPE_QUERY_TYPES,
};

unsigned d3d11_query_size[D3D11_QUERY_COUNT] =
{
		sizeof(BOOL),
		sizeof(UINT64),
		sizeof(UINT64),
		sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT),
		sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS),
		sizeof(BOOL),
		sizeof(D3D11_QUERY_DATA_SO_STATISTICS),
		sizeof(BOOL),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
};
