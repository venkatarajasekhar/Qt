/**********************************************************
 * Copyright 2009-2011 VMware, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *********************************************************
 * Authors:
 * Zack Rusin <zackr-at-vmware-dot-com>
 * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
 */

#ifndef _XA_PRIV_H_
#define _XA_PRIV_H_

#include "xa_tracker.h"
#include "xa_context.h"
#include "xa_composite.h"

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#if defined(__GNUC__) && __GNUC__ >= 4
#define XA_EXPORT __attribute__ ((visibility("default")))
#else
#define XA_EXPORT
#endif

#define XA_VB_SIZE (100 * 4 * 3 * 4)
#define XA_LAST_SURFACE_TYPE (xa_type_yuv_component + 1)
#define XA_MAX_SAMPLERS 3

struct xa_fence {
    struct pipe_fence_handle *pipe_fence;
    struct xa_tracker *xa;
};

struct xa_format_descriptor {
    enum pipe_format format;
    enum xa_formats xa_format;
};

struct xa_surface {
    struct pipe_resource template;
    struct xa_tracker *xa;
    struct pipe_resource *tex;
    struct pipe_transfer *transfer;
    unsigned int flags;
    struct xa_format_descriptor fdesc;
    struct pipe_context *mapping_pipe;
};

struct xa_tracker {
    enum xa_formats *supported_formats;
    unsigned int format_map[XA_LAST_SURFACE_TYPE][2];
    int d_depth_bits_last;
    int ds_depth_bits_last;
    struct pipe_screen *screen;
    struct xa_context *default_ctx;
};

struct xa_context {
    struct xa_tracker *xa;
    struct pipe_context *pipe;

    struct cso_context *cso;
    struct xa_shaders *shaders;

    struct pipe_resource *vs_const_buffer;
    struct pipe_resource *fs_const_buffer;

    float buffer[XA_VB_SIZE];
    unsigned int buffer_size;
    struct pipe_vertex_element velems[3];

    /* number of attributes per vertex for the current
     * draw operation */
    unsigned int attrs_per_vertex;

    unsigned int fb_width;
    unsigned int fb_height;

    struct pipe_fence_handle *last_fence;
    struct xa_surface *src;
    struct xa_surface *dst;
    struct pipe_surface *srf;

    int simple_copy;

    int has_solid_color;
    float solid_color[4];

    unsigned int num_bound_samplers;
    struct pipe_sampler_view *bound_sampler_views[XA_MAX_SAMPLERS];
    const struct xa_composite *comp;
};

enum xa_vs_traits {
    VS_COMPOSITE = 1 << 0,
    VS_MASK = 1 << 1,
    VS_SOLID_FILL = 1 << 2,
    VS_LINGRAD_FILL = 1 << 3,
    VS_RADGRAD_FILL = 1 << 4,
    VS_YUV = 1 << 5,

    VS_FILL = (VS_SOLID_FILL | VS_LINGRAD_FILL | VS_RADGRAD_FILL)
};

enum xa_fs_traits {
    FS_COMPOSITE = 1 << 0,
    FS_MASK = 1 << 1,
    FS_SOLID_FILL = 1 << 2,
    FS_LINGRAD_FILL = 1 << 3,
    FS_RADGRAD_FILL = 1 << 4,
    FS_CA_FULL = 1 << 5,	/* src.rgba * mask.rgba */
    FS_CA_SRCALPHA = 1 << 6,	/* src.aaaa * mask.rgba */
    FS_YUV = 1 << 7,
    FS_SRC_REPEAT_NONE = 1 << 8,
    FS_MASK_REPEAT_NONE = 1 << 9,
    FS_SRC_SWIZZLE_RGB = 1 << 10,
    FS_MASK_SWIZZLE_RGB = 1 << 11,
    FS_SRC_SET_ALPHA = 1 << 12,
    FS_MASK_SET_ALPHA = 1 << 13,
    FS_SRC_LUMINANCE = 1 << 14,
    FS_MASK_LUMINANCE = 1 << 15,
    FS_DST_LUMINANCE = 1 << 16,

    FS_FILL = (FS_SOLID_FILL | FS_LINGRAD_FILL | FS_RADGRAD_FILL),
    FS_COMPONENT_ALPHA = (FS_CA_FULL | FS_CA_SRCALPHA)
};

struct xa_shader {
    void *fs;
    void *vs;
};

struct xa_shaders;

/*
 * Inline utilities
 */

static INLINE int
xa_min(int a, int b)
{
    return ((a <= b) ? a : b);
}

static INLINE void
xa_pixel_to_float4(uint32_t pixel, float *color)
{
    uint32_t	    r, g, b, a;

    a = (pixel >> 24) & 0xff;
    r = (pixel >> 16) & 0xff;
    g = (pixel >>  8) & 0xff;
    b = (pixel >>  0) & 0xff;
    color[0] = ((float)r) / 255.;
    color[1] = ((float)g) / 255.;
    color[2] = ((float)b) / 255.;
    color[3] = ((float)a) / 255.;
}

static INLINE void
xa_pixel_to_float4_a8(uint32_t pixel, float *color)
{
    uint32_t a;

    a = (pixel >> 24) & 0xff;
    color[0] = ((float)a) / 255.;
    color[1] = ((float)a) / 255.;
    color[2] = ((float)a) / 255.;
    color[3] = ((float)a) / 255.;
}

/*
 * xa_tgsi.c
 */

extern struct xa_shaders *xa_shaders_create(struct xa_context *);

void xa_shaders_destroy(struct xa_shaders *shaders);

struct xa_shader xa_shaders_get(struct xa_shaders *shaders,
				unsigned vs_traits, unsigned fs_traits);

/*
 * xa_context.c
 */
extern int
xa_ctx_srf_create(struct xa_context *ctx, struct xa_surface *dst);

extern void
xa_ctx_srf_destroy(struct xa_context *ctx);

extern void
xa_ctx_sampler_views_destroy(struct xa_context *ctx);

/*
 * xa_renderer.c
 */
void renderer_set_constants(struct xa_context *r,
			    int shader_type, const float *params,
			    int param_bytes);

void renderer_draw_yuv(struct xa_context *r,
		       float src_x,
		       float src_y,
		       float src_w,
		       float src_h,
		       int dst_x,
		       int dst_y, int dst_w, int dst_h,
		       struct xa_surface *srf[]);

void renderer_bind_destination(struct xa_context *r,
			       struct pipe_surface *surface, int width,
			       int height);

void renderer_init_state(struct xa_context *r);
void renderer_copy_prepare(struct xa_context *r,
			   struct pipe_surface *dst_surface,
			   struct pipe_resource *src_texture,
			   const enum xa_formats src_xa_format,
			   const enum xa_formats dst_xa_format);

void renderer_copy(struct xa_context *r, int dx,
		   int dy,
		   int sx,
		   int sy,
		   int width, int height, float src_width, float src_height);

void renderer_draw_flush(struct xa_context *r);

void renderer_begin_solid(struct xa_context *r);
void renderer_solid(struct xa_context *r,
		    int x0, int y0, int x1, int y1, float *color);
void
renderer_begin_textures(struct xa_context *r);

void
renderer_texture(struct xa_context *r,
		 int *pos,
		 int width, int height,
		 const float *src_matrix,
		 const float *mask_matrix);

#endif
