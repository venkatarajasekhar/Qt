/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * The rast code is concerned with rasterization of command bins.
 * Each screen tile has a bin associated with it.  To render the
 * scene we iterate over the tile bins and execute the commands
 * in each bin.
 * We'll do that with multiple threads...
 */


#ifndef LP_RAST_H
#define LP_RAST_H

#include "pipe/p_compiler.h"
#include "lp_jit.h"


struct lp_rasterizer;
struct lp_scene;
struct lp_fence;
struct cmd_bin;

/** For sub-pixel positioning */
#define FIXED_ORDER 4
#define FIXED_ONE (1<<FIXED_ORDER)


struct lp_rasterizer_task;


/**
 * Rasterization state.
 * Objects of this type are put into the shared data bin and pointed
 * to by commands in the per-tile bins.
 */
struct lp_rast_state {
   /* State for the shader.  This also contains state which feeds into
    * the fragment shader, such as blend color and alpha ref value.
    */
   struct lp_jit_context jit_context;
   
   /* The shader itself.  Probably we also need to pass a pointer to
    * the tile color/z/stencil data somehow
     */
   struct lp_fragment_shader_variant *variant;
};


/**
 * Coefficients necessary to run the shader at a given location.
 * First coefficient is position.
 * These pointers point into the bin data buffer.
 */
struct lp_rast_shader_inputs {
   unsigned frontfacing:1;      /** True for front-facing */
   unsigned disable:1;          /** Partially binned, disable this command */
   unsigned opaque:1;           /** Is opaque */
   unsigned pad0:29;            /* wasted space */
   unsigned stride;             /* how much to advance data between a0, dadx, dady */
   unsigned pad2;               /* wasted space */
   unsigned pad3;               /* wasted space */
   /* followed by a0, dadx, dady and planes[] */
};

/* Note: the order of these values is important as they are loaded by
 * sse code in rasterization:
 */
struct lp_rast_plane {
   /* edge function values at minx,miny ?? */
   int c;

   int dcdx;
   int dcdy;

   /* one-pixel sized trivial reject offsets for each plane */
   int eo;
};

/**
 * Rasterization information for a triangle known to be in this bin,
 * plus inputs to run the shader:
 * These fields are tile- and bin-independent.
 * Objects of this type are put into the lp_setup_context::data buffer.
 */
struct lp_rast_triangle {
#ifdef DEBUG
   float v[3][2];
   float pad0;
   float pad1;
#endif

   /* inputs for the shader */
   struct lp_rast_shader_inputs inputs;
   /* planes are also allocated here */
};


#define GET_A0(inputs) ((float (*)[4])((inputs)+1))
#define GET_DADX(inputs) ((float (*)[4])((char *)((inputs) + 1) + (inputs)->stride))
#define GET_DADY(inputs) ((float (*)[4])((char *)((inputs) + 1) + 2 * (inputs)->stride))
#define GET_PLANES(tri) ((struct lp_rast_plane *)((char *)(&(tri)->inputs + 1) + 3 * (tri)->inputs.stride))



struct lp_rasterizer *
lp_rast_create( unsigned num_threads );

void
lp_rast_destroy( struct lp_rasterizer * );

unsigned
lp_rast_get_num_threads( struct lp_rasterizer * );

void 
lp_rast_queue_scene( struct lp_rasterizer *rast,
                     struct lp_scene *scene );

void
lp_rast_finish( struct lp_rasterizer *rast );


union lp_rast_cmd_arg {
   const struct lp_rast_shader_inputs *shade_tile;
   struct {
      const struct lp_rast_triangle *tri;
      unsigned plane_mask;
   } triangle;
   const struct lp_rast_state *set_state;
   uint8_t clear_color[4];
   struct {
      uint32_t value;
      uint32_t mask;
   } clear_zstencil;
   const struct lp_rast_state *state;
   struct lp_fence *fence;
   struct llvmpipe_query *query_obj;
};


/* Cast wrappers.  Hopefully these compile to noops!
 */
static INLINE union lp_rast_cmd_arg
lp_rast_arg_inputs( const struct lp_rast_shader_inputs *shade_tile )
{
   union lp_rast_cmd_arg arg;
   arg.shade_tile = shade_tile;
   return arg;
}

static INLINE union lp_rast_cmd_arg
lp_rast_arg_triangle( const struct lp_rast_triangle *triangle,
                      unsigned plane_mask)
{
   union lp_rast_cmd_arg arg;
   arg.triangle.tri = triangle;
   arg.triangle.plane_mask = plane_mask;
   return arg;
}

/**
 * Build argument for a contained triangle.
 *
 * All planes are enabled, so instead of the plane mask we pass the upper
 * left coordinates of the a block that fully encloses the triangle.
 */
static INLINE union lp_rast_cmd_arg
lp_rast_arg_triangle_contained( const struct lp_rast_triangle *triangle,
                                unsigned x, unsigned y)
{
   union lp_rast_cmd_arg arg;
   arg.triangle.tri = triangle;
   arg.triangle.plane_mask = x | (y << 8);
   return arg;
}

static INLINE union lp_rast_cmd_arg
lp_rast_arg_state( const struct lp_rast_state *state )
{
   union lp_rast_cmd_arg arg;
   arg.set_state = state;
   return arg;
}

static INLINE union lp_rast_cmd_arg
lp_rast_arg_fence( struct lp_fence *fence )
{
   union lp_rast_cmd_arg arg;
   arg.fence = fence;
   return arg;
}


static INLINE union lp_rast_cmd_arg
lp_rast_arg_clearzs( unsigned value, unsigned mask )
{
   union lp_rast_cmd_arg arg;
   arg.clear_zstencil.value = value;
   arg.clear_zstencil.mask = mask;
   return arg;
}


static INLINE union lp_rast_cmd_arg
lp_rast_arg_query( struct llvmpipe_query *pq )
{
   union lp_rast_cmd_arg arg;
   arg.query_obj = pq;
   return arg;
}

static INLINE union lp_rast_cmd_arg
lp_rast_arg_null( void )
{
   union lp_rast_cmd_arg arg;
   arg.set_state = NULL;
   return arg;
}


/**
 * Binnable Commands.
 * These get put into bins by the setup code and are called when
 * the bins are executed.
 */
#define LP_RAST_OP_CLEAR_COLOR       0x0
#define LP_RAST_OP_CLEAR_ZSTENCIL    0x1
#define LP_RAST_OP_TRIANGLE_1        0x2
#define LP_RAST_OP_TRIANGLE_2        0x3
#define LP_RAST_OP_TRIANGLE_3        0x4
#define LP_RAST_OP_TRIANGLE_4        0x5
#define LP_RAST_OP_TRIANGLE_5        0x6
#define LP_RAST_OP_TRIANGLE_6        0x7
#define LP_RAST_OP_TRIANGLE_7        0x8
#define LP_RAST_OP_TRIANGLE_8        0x9
#define LP_RAST_OP_TRIANGLE_3_4      0xa
#define LP_RAST_OP_TRIANGLE_3_16     0xb
#define LP_RAST_OP_TRIANGLE_4_16     0xc
#define LP_RAST_OP_SHADE_TILE        0xd
#define LP_RAST_OP_SHADE_TILE_OPAQUE 0xe
#define LP_RAST_OP_BEGIN_QUERY       0xf
#define LP_RAST_OP_END_QUERY         0x10
#define LP_RAST_OP_SET_STATE         0x11

#define LP_RAST_OP_MAX               0x12
#define LP_RAST_OP_MASK              0xff

void
lp_debug_bins( struct lp_scene *scene );
void
lp_debug_draw_bins_by_cmd_length( struct lp_scene *scene );
void
lp_debug_draw_bins_by_coverage( struct lp_scene *scene );


#endif
