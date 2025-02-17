/**************************************************************************

Copyright 2000, 2001 VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/api_arrayelt.h"
#include "main/enums.h"
#include "main/light.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/fbobject.h"
#include "main/simple_list.h"
#include "main/state.h"

#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"
#include "drivers/common/meta.h"

#include "radeon_context.h"
#include "radeon_mipmap_tree.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_tcl.h"
#include "radeon_tex.h"
#include "radeon_swtcl.h"

static void radeonUpdateSpecular( struct gl_context *ctx );

/* =============================================================
 * Alpha blending
 */

static void radeonAlphaFunc( struct gl_context *ctx, GLenum func, GLfloat ref )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   int pp_misc = rmesa->hw.ctx.cmd[CTX_PP_MISC];
   GLubyte refByte;

   CLAMPED_FLOAT_TO_UBYTE(refByte, ref);

   RADEON_STATECHANGE( rmesa, ctx );

   pp_misc &= ~(RADEON_ALPHA_TEST_OP_MASK | RADEON_REF_ALPHA_MASK);
   pp_misc |= (refByte & RADEON_REF_ALPHA_MASK);

   switch ( func ) {
   case GL_NEVER:
      pp_misc |= RADEON_ALPHA_TEST_FAIL;
      break;
   case GL_LESS:
      pp_misc |= RADEON_ALPHA_TEST_LESS;
      break;
   case GL_EQUAL:
      pp_misc |= RADEON_ALPHA_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      pp_misc |= RADEON_ALPHA_TEST_LEQUAL;
      break;
   case GL_GREATER:
      pp_misc |= RADEON_ALPHA_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      pp_misc |= RADEON_ALPHA_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      pp_misc |= RADEON_ALPHA_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      pp_misc |= RADEON_ALPHA_TEST_PASS;
      break;
   }

   rmesa->hw.ctx.cmd[CTX_PP_MISC] = pp_misc;
}

static void radeonBlendEquationSeparate( struct gl_context *ctx,
					 GLenum modeRGB, GLenum modeA )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint b = rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] & ~RADEON_COMB_FCN_MASK;
   GLboolean fallback = GL_FALSE;

   assert( modeRGB == modeA );

   switch ( modeRGB ) {
   case GL_FUNC_ADD:
   case GL_LOGIC_OP:
      b |= RADEON_COMB_FCN_ADD_CLAMP;
      break;

   case GL_FUNC_SUBTRACT:
      b |= RADEON_COMB_FCN_SUB_CLAMP;
      break;

   default:
      if (ctx->Color.BlendEnabled)
	 fallback = GL_TRUE;
      else
	 b |= RADEON_COMB_FCN_ADD_CLAMP;
      break;
   }

   FALLBACK( rmesa, RADEON_FALLBACK_BLEND_EQ, fallback );
   if ( !fallback ) {
      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = b;
      if ( (ctx->Color.ColorLogicOpEnabled || (ctx->Color.BlendEnabled
	    && ctx->Color.Blend[0].EquationRGB == GL_LOGIC_OP)) ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_ROP_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_ROP_ENABLE;
      }
   }
}

static void radeonBlendFuncSeparate( struct gl_context *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint b = rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] &
      ~(RADEON_SRC_BLEND_MASK | RADEON_DST_BLEND_MASK);
   GLboolean fallback = GL_FALSE;

   switch ( ctx->Color.Blend[0].SrcRGB ) {
   case GL_ZERO:
      b |= RADEON_SRC_BLEND_GL_ZERO;
      break;
   case GL_ONE:
      b |= RADEON_SRC_BLEND_GL_ONE;
      break;
   case GL_DST_COLOR:
      b |= RADEON_SRC_BLEND_GL_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_DST_COLOR;
      break;
   case GL_SRC_COLOR:
      b |= RADEON_SRC_BLEND_GL_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      b |= RADEON_SRC_BLEND_GL_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      b |= RADEON_SRC_BLEND_GL_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      b |= RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA;
      break;
   case GL_SRC_ALPHA_SATURATE:
      b |= RADEON_SRC_BLEND_GL_SRC_ALPHA_SATURATE;
      break;
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      if (ctx->Color.BlendEnabled)
	 fallback = GL_TRUE;
      else
	 b |= RADEON_SRC_BLEND_GL_ONE;
      break;
   default:
      break;
   }

   switch ( ctx->Color.Blend[0].DstRGB ) {
   case GL_ZERO:
      b |= RADEON_DST_BLEND_GL_ZERO;
      break;
   case GL_ONE:
      b |= RADEON_DST_BLEND_GL_ONE;
      break;
   case GL_SRC_COLOR:
      b |= RADEON_DST_BLEND_GL_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      b |= RADEON_DST_BLEND_GL_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_COLOR:
      b |= RADEON_DST_BLEND_GL_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_DST_COLOR;
      break;
   case GL_DST_ALPHA:
      b |= RADEON_DST_BLEND_GL_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      b |= RADEON_DST_BLEND_GL_ONE_MINUS_DST_ALPHA;
      break;
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      if (ctx->Color.BlendEnabled)
	 fallback = GL_TRUE;
      else
	 b |= RADEON_DST_BLEND_GL_ZERO;
      break;
   default:
      break;
   }

   FALLBACK( rmesa, RADEON_FALLBACK_BLEND_FUNC, fallback );
   if ( !fallback ) {
      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = b;
   }
}


/* =============================================================
 * Depth testing
 */

static void radeonDepthFunc( struct gl_context *ctx, GLenum func )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~RADEON_Z_TEST_MASK;

   switch ( ctx->Depth.Func ) {
   case GL_NEVER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_TEST_ALWAYS;
      break;
   }
}


static void radeonDepthMask( struct gl_context *ctx, GLboolean flag )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   RADEON_STATECHANGE( rmesa, ctx );

   if ( ctx->Depth.Mask ) {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |=  RADEON_Z_WRITE_ENABLE;
   } else {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~RADEON_Z_WRITE_ENABLE;
   }
}


/* =============================================================
 * Fog
 */


static void radeonFogfv( struct gl_context *ctx, GLenum pname, const GLfloat *param )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   union { int i; float f; } c, d;
   GLubyte col[4];

   switch (pname) {
   case GL_FOG_MODE:
      if (!ctx->Fog.Enabled)
	 return;
      RADEON_STATECHANGE(rmesa, tcl);
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~RADEON_TCL_FOG_MASK;
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_TCL_FOG_LINEAR;
	 break;
      case GL_EXP:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_TCL_FOG_EXP;
	 break;
      case GL_EXP2:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_TCL_FOG_EXP2;
	 break;
      default:
	 return;
      }
   /* fallthrough */
   case GL_FOG_DENSITY:
   case GL_FOG_START:
   case GL_FOG_END:
      if (!ctx->Fog.Enabled)
	 return;
      c.i = rmesa->hw.fog.cmd[FOG_C];
      d.i = rmesa->hw.fog.cmd[FOG_D];
      switch (ctx->Fog.Mode) {
      case GL_EXP:
	 c.f = 0.0;
	 /* While this is the opposite sign from the DDK, it makes the fog test
	  * pass, and matches r200.
	  */
	 d.f = -ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      case GL_LINEAR:
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 } else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    /* While this is the opposite sign from the DDK, it makes the fog
	     * test pass, and matches r200.
	     */
	    d.f = -1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
	 break;
      default:
	 break;
      }
      if (c.i != rmesa->hw.fog.cmd[FOG_C] || d.i != rmesa->hw.fog.cmd[FOG_D]) {
	 RADEON_STATECHANGE( rmesa, fog );
	 rmesa->hw.fog.cmd[FOG_C] = c.i;
	 rmesa->hw.fog.cmd[FOG_D] = d.i;
      }
      break;
   case GL_FOG_COLOR:
      RADEON_STATECHANGE( rmesa, ctx );
      _mesa_unclamped_float_rgba_to_ubyte(col, ctx->Fog.Color );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~RADEON_FOG_COLOR_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |=
	 radeonPackColor( 4, col[0], col[1], col[2], 0 );
      break;
   case GL_FOG_COORD_SRC:
      radeonUpdateSpecular( ctx );
      break;
   default:
      return;
   }
}

/* =============================================================
 * Culling
 */

static void radeonCullFace( struct gl_context *ctx, GLenum unused )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint s = rmesa->hw.set.cmd[SET_SE_CNTL];
   GLuint t = rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL];

   s |= RADEON_FFACE_SOLID | RADEON_BFACE_SOLID;
   t &= ~(RADEON_CULL_FRONT | RADEON_CULL_BACK);

   if ( ctx->Polygon.CullFlag ) {
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 s &= ~RADEON_FFACE_SOLID;
	 t |= RADEON_CULL_FRONT;
	 break;
      case GL_BACK:
	 s &= ~RADEON_BFACE_SOLID;
	 t |= RADEON_CULL_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 s &= ~(RADEON_FFACE_SOLID | RADEON_BFACE_SOLID);
	 t |= (RADEON_CULL_FRONT | RADEON_CULL_BACK);
	 break;
      }
   }

   if ( rmesa->hw.set.cmd[SET_SE_CNTL] != s ) {
      RADEON_STATECHANGE(rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = s;
   }

   if ( rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] != t ) {
      RADEON_STATECHANGE(rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] = t;
   }
}

static void radeonFrontFace( struct gl_context *ctx, GLenum mode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   int cull_face = (mode == GL_CW) ? RADEON_FFACE_CULL_CW : RADEON_FFACE_CULL_CCW;

   RADEON_STATECHANGE( rmesa, set );
   rmesa->hw.set.cmd[SET_SE_CNTL] &= ~RADEON_FFACE_CULL_DIR_MASK;

   RADEON_STATECHANGE( rmesa, tcl );
   rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~RADEON_CULL_FRONT_IS_CCW;

   /* Winding is inverted when rendering to FBO */
   if (ctx->DrawBuffer && _mesa_is_user_fbo(ctx->DrawBuffer))
      cull_face = (mode == GL_CCW) ? RADEON_FFACE_CULL_CW : RADEON_FFACE_CULL_CCW;
   rmesa->hw.set.cmd[SET_SE_CNTL] |= cull_face;

   if ( mode == GL_CCW )
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_CULL_FRONT_IS_CCW;
}


/* =============================================================
 * Line state
 */
static void radeonLineWidth( struct gl_context *ctx, GLfloat widthf )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, lin );
   RADEON_STATECHANGE( rmesa, set );

   /* Line width is stored in U6.4 format.
    */
   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] = (GLuint)(widthf * 16.0);
   if ( widthf > 1.0 ) {
      rmesa->hw.set.cmd[SET_SE_CNTL] |=  RADEON_WIDELINE_ENABLE;
   } else {
      rmesa->hw.set.cmd[SET_SE_CNTL] &= ~RADEON_WIDELINE_ENABLE;
   }
}

static void radeonLineStipple( struct gl_context *ctx, GLint factor, GLushort pattern )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, lin );
   rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] =
      ((((GLuint)factor & 0xff) << 16) | ((GLuint)pattern));
}


/* =============================================================
 * Masks
 */
static void radeonColorMask( struct gl_context *ctx,
			     GLboolean r, GLboolean g,
			     GLboolean b, GLboolean a )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb;
   GLuint mask;

   rrb = radeon_get_colorbuffer(&rmesa->radeon);
   if (!rrb)
     return;

   mask = radeonPackColor( rrb->cpp,
			   ctx->Color.ColorMask[0][RCOMP],
			   ctx->Color.ColorMask[0][GCOMP],
			   ctx->Color.ColorMask[0][BCOMP],
			   ctx->Color.ColorMask[0][ACOMP] );

   if ( rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] != mask ) {
      RADEON_STATECHANGE( rmesa, msk );
      rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] = mask;
   }
}


/* =============================================================
 * Polygon state
 */

static void radeonPolygonOffset( struct gl_context *ctx,
				 GLfloat factor, GLfloat units )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   const GLfloat depthScale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   float_ui32_type constant =  { units * depthScale };
   float_ui32_type factoru = { factor };

   RADEON_STATECHANGE( rmesa, zbs );
   rmesa->hw.zbs.cmd[ZBS_SE_ZBIAS_FACTOR]   = factoru.ui32;
   rmesa->hw.zbs.cmd[ZBS_SE_ZBIAS_CONSTANT] = constant.ui32;
}

static void radeonPolygonMode( struct gl_context *ctx, GLenum face, GLenum mode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLboolean flag = (ctx->_TriangleCaps & DD_TRI_UNFILLED) != 0;

   /* Can't generally do unfilled via tcl, but some good special
    * cases work.
    */
   TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_UNFILLED, flag);
   if (rmesa->radeon.TclFallback) {
      radeonChooseRenderState( ctx );
      radeonChooseVertexState( ctx );
   }
}


/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

/* Examine lighting and texture state to determine if separate specular
 * should be enabled.
 */
static void radeonUpdateSpecular( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   uint32_t p = rmesa->hw.ctx.cmd[CTX_PP_CNTL];
   GLuint flag = 0;

   RADEON_STATECHANGE( rmesa, tcl );

   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_SPECULAR;
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &= ~RADEON_TCL_COMPUTE_DIFFUSE;
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~RADEON_TCL_VTX_PK_SPEC;
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~RADEON_TCL_VTX_PK_DIFFUSE;
   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_LIGHTING_ENABLE;

   p &= ~RADEON_SPECULAR_ENABLE;

   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_DIFFUSE_SPECULAR_COMBINE;


   if (ctx->Light.Enabled &&
       ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_SPECULAR;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
      p |=  RADEON_SPECULAR_ENABLE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &=
	 ~RADEON_DIFFUSE_SPECULAR_COMBINE;
   }
   else if (ctx->Light.Enabled) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
   } else if (ctx->Fog.ColorSumEnabled ) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
      p |= RADEON_SPECULAR_ENABLE;
   } else {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_DIFFUSE;
   }

   if (ctx->Fog.Enabled) {
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_PK_SPEC;
      if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH) {
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] |= RADEON_TCL_COMPUTE_SPECULAR;
      /* Bizzare: have to leave lighting enabled to get fog. */
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LIGHTING_ENABLE;
      }
      else {
      /* cannot do tcl fog factor calculation with fog coord source
       * (send precomputed factors). Cannot use precomputed fog
       * factors together with tcl spec light (need tcl fallback) */
	 flag = (rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] &
	    RADEON_TCL_COMPUTE_SPECULAR) != 0;
      }
   }

   TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_FOGCOORDSPEC, flag);

   if (_mesa_need_secondary_color(ctx)) {
      assert( (p & RADEON_SPECULAR_ENABLE) != 0 );
   } else {
      assert( (p & RADEON_SPECULAR_ENABLE) == 0 );
   }

   if ( rmesa->hw.ctx.cmd[CTX_PP_CNTL] != p ) {
      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] = p;
   }

   /* Update vertex/render formats
    */
   if (rmesa->radeon.TclFallback) {
      radeonChooseRenderState( ctx );
      radeonChooseVertexState( ctx );
   }
}


/* =============================================================
 * Materials
 */


/* Update on colormaterial, material emmissive/ambient,
 * lightmodel.globalambient
 */
static void update_global_ambient( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   float *fcmd = (float *)RADEON_DB_STATE( glt );

   /* Need to do more if both emmissive & ambient are PREMULT:
    * Hope this is not needed for MULT
    */
   if ((rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &
       ((3 << RADEON_EMISSIVE_SOURCE_SHIFT) |
	(3 << RADEON_AMBIENT_SOURCE_SHIFT))) == 0)
   {
      COPY_3V( &fcmd[GLT_RED],
	       ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION]);
      ACC_SCALE_3V( &fcmd[GLT_RED],
		   ctx->Light.Model.Ambient,
		   ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT]);
   }
   else
   {
      COPY_3V( &fcmd[GLT_RED], ctx->Light.Model.Ambient );
   }

   RADEON_DB_STATECHANGE(rmesa, &rmesa->hw.glt);
}

/* Update on change to
 *    - light[p].colors
 *    - light[p].enabled
 */
static void update_light_colors( struct gl_context *ctx, GLuint p )
{
   struct gl_light *l = &ctx->Light.Light[p];

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (l->Enabled) {
      r100ContextPtr rmesa = R100_CONTEXT(ctx);
      float *fcmd = (float *)RADEON_DB_STATE( lit[p] );

      COPY_4V( &fcmd[LIT_AMBIENT_RED], l->Ambient );
      COPY_4V( &fcmd[LIT_DIFFUSE_RED], l->Diffuse );
      COPY_4V( &fcmd[LIT_SPECULAR_RED], l->Specular );

      RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
   }
}

/* Also fallback for asym colormaterial mode in twoside lighting...
 */
static void check_twoside_fallback( struct gl_context *ctx )
{
   GLboolean fallback = GL_FALSE;
   GLint i;

   if (ctx->Light.Enabled && ctx->Light.Model.TwoSide) {
      if (ctx->Light.ColorMaterialEnabled &&
	  (ctx->Light._ColorMaterialBitmask & BACK_MATERIAL_BITS) !=
	  ((ctx->Light._ColorMaterialBitmask & FRONT_MATERIAL_BITS)<<1))
	 fallback = GL_TRUE;
      else {
	 for (i = MAT_ATTRIB_FRONT_AMBIENT; i < MAT_ATTRIB_FRONT_INDEXES; i+=2)
	    if (memcmp( ctx->Light.Material.Attrib[i],
			ctx->Light.Material.Attrib[i+1],
			sizeof(GLfloat)*4) != 0) {
	       fallback = GL_TRUE;
	       break;
	    }
      }
   }

   TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_LIGHT_TWOSIDE, fallback );
}


static void radeonColorMaterial( struct gl_context *ctx, GLenum face, GLenum mode )
{
      r100ContextPtr rmesa = R100_CONTEXT(ctx);
      GLuint light_model_ctl1 = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL];

      light_model_ctl1 &= ~((3 << RADEON_EMISSIVE_SOURCE_SHIFT) |
			   (3 << RADEON_AMBIENT_SOURCE_SHIFT) |
			   (3 << RADEON_DIFFUSE_SOURCE_SHIFT) |
			   (3 << RADEON_SPECULAR_SOURCE_SHIFT));

   if (ctx->Light.ColorMaterialEnabled) {
      GLuint mask = ctx->Light._ColorMaterialBitmask;

      if (mask & MAT_BIT_FRONT_EMISSION) {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_EMISSIVE_SOURCE_SHIFT);
      }
      else {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_STATE_MULT <<
			     RADEON_EMISSIVE_SOURCE_SHIFT);
      }

      if (mask & MAT_BIT_FRONT_AMBIENT) {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_AMBIENT_SOURCE_SHIFT);
      }
      else {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_STATE_MULT <<
			     RADEON_AMBIENT_SOURCE_SHIFT);
      }

      if (mask & MAT_BIT_FRONT_DIFFUSE) {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_DIFFUSE_SOURCE_SHIFT);
      }
      else {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_STATE_MULT <<
			     RADEON_DIFFUSE_SOURCE_SHIFT);
      }

      if (mask & MAT_BIT_FRONT_SPECULAR) {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_VERTEX_DIFFUSE <<
			     RADEON_SPECULAR_SOURCE_SHIFT);
      }
      else {
	 light_model_ctl1 |= (RADEON_LM_SOURCE_STATE_MULT <<
			     RADEON_SPECULAR_SOURCE_SHIFT);
      }
   }
   else {
   /* Default to MULT:
    */
      light_model_ctl1 |= (RADEON_LM_SOURCE_STATE_MULT << RADEON_EMISSIVE_SOURCE_SHIFT) |
		   (RADEON_LM_SOURCE_STATE_MULT << RADEON_AMBIENT_SOURCE_SHIFT) |
		   (RADEON_LM_SOURCE_STATE_MULT << RADEON_DIFFUSE_SOURCE_SHIFT) |
		   (RADEON_LM_SOURCE_STATE_MULT << RADEON_SPECULAR_SOURCE_SHIFT);
   }

      if (light_model_ctl1 != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL]) {
	 RADEON_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] = light_model_ctl1;
   }
}

void radeonUpdateMaterial( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
   GLfloat *fcmd = (GLfloat *)RADEON_DB_STATE( mtl );
   GLuint mask = ~0;

   if (ctx->Light.ColorMaterialEnabled)
      mask &= ~ctx->Light._ColorMaterialBitmask;

   if (RADEON_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s\n", __FUNCTION__);


   if (mask & MAT_BIT_FRONT_EMISSION) {
      fcmd[MTL_EMMISSIVE_RED]   = mat[MAT_ATTRIB_FRONT_EMISSION][0];
      fcmd[MTL_EMMISSIVE_GREEN] = mat[MAT_ATTRIB_FRONT_EMISSION][1];
      fcmd[MTL_EMMISSIVE_BLUE]  = mat[MAT_ATTRIB_FRONT_EMISSION][2];
      fcmd[MTL_EMMISSIVE_ALPHA] = mat[MAT_ATTRIB_FRONT_EMISSION][3];
   }
   if (mask & MAT_BIT_FRONT_AMBIENT) {
      fcmd[MTL_AMBIENT_RED]     = mat[MAT_ATTRIB_FRONT_AMBIENT][0];
      fcmd[MTL_AMBIENT_GREEN]   = mat[MAT_ATTRIB_FRONT_AMBIENT][1];
      fcmd[MTL_AMBIENT_BLUE]    = mat[MAT_ATTRIB_FRONT_AMBIENT][2];
      fcmd[MTL_AMBIENT_ALPHA]   = mat[MAT_ATTRIB_FRONT_AMBIENT][3];
   }
   if (mask & MAT_BIT_FRONT_DIFFUSE) {
      fcmd[MTL_DIFFUSE_RED]     = mat[MAT_ATTRIB_FRONT_DIFFUSE][0];
      fcmd[MTL_DIFFUSE_GREEN]   = mat[MAT_ATTRIB_FRONT_DIFFUSE][1];
      fcmd[MTL_DIFFUSE_BLUE]    = mat[MAT_ATTRIB_FRONT_DIFFUSE][2];
      fcmd[MTL_DIFFUSE_ALPHA]   = mat[MAT_ATTRIB_FRONT_DIFFUSE][3];
   }
   if (mask & MAT_BIT_FRONT_SPECULAR) {
      fcmd[MTL_SPECULAR_RED]    = mat[MAT_ATTRIB_FRONT_SPECULAR][0];
      fcmd[MTL_SPECULAR_GREEN]  = mat[MAT_ATTRIB_FRONT_SPECULAR][1];
      fcmd[MTL_SPECULAR_BLUE]   = mat[MAT_ATTRIB_FRONT_SPECULAR][2];
      fcmd[MTL_SPECULAR_ALPHA]  = mat[MAT_ATTRIB_FRONT_SPECULAR][3];
   }
   if (mask & MAT_BIT_FRONT_SHININESS) {
      fcmd[MTL_SHININESS]       = mat[MAT_ATTRIB_FRONT_SHININESS][0];
   }

   RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.mtl );

   check_twoside_fallback( ctx );
/*   update_global_ambient( ctx );*/
}

/* _NEW_LIGHT
 * _NEW_MODELVIEW
 * _MESA_NEW_NEED_EYE_COORDS
 *
 * Uses derived state from mesa:
 *       _VP_inf_norm
 *       _h_inf_norm
 *       _Position
 *       _NormSpotDirection
 *       _ModelViewInvScale
 *       _NeedEyeCoords
 *       _EyeZDir
 *
 * which are calculated in light.c and are correct for the current
 * lighting space (model or eye), hence dependencies on _NEW_MODELVIEW
 * and _MESA_NEW_NEED_EYE_COORDS.
 */
static void update_light( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   /* Have to check these, or have an automatic shortcircuit mechanism
    * to remove noop statechanges. (Or just do a better job on the
    * front end).
    */
   {
      GLuint tmp = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL];

      if (ctx->_NeedEyeCoords)
	 tmp &= ~RADEON_LIGHT_IN_MODELSPACE;
      else
	 tmp |= RADEON_LIGHT_IN_MODELSPACE;


      /* Leave this test disabled: (unexplained q3 lockup) (even with
         new packets)
      */
      if (tmp != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL])
      {
	 RADEON_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] = tmp;
      }
   }

   {
      GLfloat *fcmd = (GLfloat *)RADEON_DB_STATE( eye );
      fcmd[EYE_X] = ctx->_EyeZDir[0];
      fcmd[EYE_Y] = ctx->_EyeZDir[1];
      fcmd[EYE_Z] = - ctx->_EyeZDir[2];
      fcmd[EYE_RESCALE_FACTOR] = ctx->_ModelViewInvScale;
      RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.eye );
   }



   if (ctx->Light.Enabled) {
      GLint p;
      for (p = 0 ; p < MAX_LIGHTS; p++) {
	 if (ctx->Light.Light[p].Enabled) {
	    struct gl_light *l = &ctx->Light.Light[p];
	    GLfloat *fcmd = (GLfloat *)RADEON_DB_STATE( lit[p] );

	    if (l->EyePosition[3] == 0.0) {
	       COPY_3FV( &fcmd[LIT_POSITION_X], l->_VP_inf_norm );
	       COPY_3FV( &fcmd[LIT_DIRECTION_X], l->_h_inf_norm );
	       fcmd[LIT_POSITION_W] = 0;
	       fcmd[LIT_DIRECTION_W] = 0;
	    } else {
	       COPY_4V( &fcmd[LIT_POSITION_X], l->_Position );
	       fcmd[LIT_DIRECTION_X] = -l->_NormSpotDirection[0];
	       fcmd[LIT_DIRECTION_Y] = -l->_NormSpotDirection[1];
	       fcmd[LIT_DIRECTION_Z] = -l->_NormSpotDirection[2];
	       fcmd[LIT_DIRECTION_W] = 0;
	    }

	    RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
	 }
      }
   }
}

static void radeonLightfv( struct gl_context *ctx, GLenum light,
			   GLenum pname, const GLfloat *params )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLint p = light - GL_LIGHT0;
   struct gl_light *l = &ctx->Light.Light[p];
   GLfloat *fcmd = (GLfloat *)rmesa->hw.lit[p].cmd;


   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
      update_light_colors( ctx, p );
      break;

   case GL_SPOT_DIRECTION:
      /* picked up in update_light */
      break;

   case GL_POSITION: {
      /* positions picked up in update_light, but can do flag here */
      GLuint flag;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      /* FIXME: Set RANGE_ATTEN only when needed */
      if (p&1)
	 flag = RADEON_LIGHT_1_IS_LOCAL;
      else
	 flag = RADEON_LIGHT_0_IS_LOCAL;

      RADEON_STATECHANGE(rmesa, tcl);
      if (l->EyePosition[3] != 0.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;
      break;
   }

   case GL_SPOT_EXPONENT:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_EXPONENT] = params[0];
      break;

   case GL_SPOT_CUTOFF: {
      GLuint flag = (p&1) ? RADEON_LIGHT_1_IS_SPOT : RADEON_LIGHT_0_IS_SPOT;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_CUTOFF] = l->_CosCutoff;

      RADEON_STATECHANGE(rmesa, tcl);
      if (l->SpotCutoff != 180.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;

      break;
   }

   case GL_CONSTANT_ATTENUATION:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_CONST] = params[0];
      if ( params[0] == 0.0 )
	 fcmd[LIT_ATTEN_CONST_INV] = FLT_MAX;
      else
	 fcmd[LIT_ATTEN_CONST_INV] = 1.0 / params[0];
      break;
   case GL_LINEAR_ATTENUATION:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_LINEAR] = params[0];
      break;
   case GL_QUADRATIC_ATTENUATION:
      RADEON_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_QUADRATIC] = params[0];
      break;
   default:
      return;
   }

   /* Set RANGE_ATTEN only when needed */
   switch (pname) {
   case GL_POSITION:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION:
   {
      GLuint *icmd = (GLuint *)RADEON_DB_STATE( tcl );
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;
      GLuint atten_flag = ( p&1 ) ? RADEON_LIGHT_1_ENABLE_RANGE_ATTEN
				  : RADEON_LIGHT_0_ENABLE_RANGE_ATTEN;
      GLuint atten_const_flag = ( p&1 ) ? RADEON_LIGHT_1_CONSTANT_RANGE_ATTEN
				  : RADEON_LIGHT_0_CONSTANT_RANGE_ATTEN;

      if ( l->EyePosition[3] == 0.0F ||
	   ( ( fcmd[LIT_ATTEN_CONST] == 0.0 || fcmd[LIT_ATTEN_CONST] == 1.0 ) &&
	     fcmd[LIT_ATTEN_QUADRATIC] == 0.0 && fcmd[LIT_ATTEN_LINEAR] == 0.0 ) ) {
	 /* Disable attenuation */
	 icmd[idx] &= ~atten_flag;
      } else {
	 if ( fcmd[LIT_ATTEN_QUADRATIC] == 0.0 && fcmd[LIT_ATTEN_LINEAR] == 0.0 ) {
	    /* Enable only constant portion of attenuation calculation */
	    icmd[idx] |= ( atten_flag | atten_const_flag );
	 } else {
	    /* Enable full attenuation calculation */
	    icmd[idx] &= ~atten_const_flag;
	    icmd[idx] |= atten_flag;
	 }
      }

      RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.tcl );
      break;
   }
   default:
      break;
   }
}




static void radeonLightModelfv( struct gl_context *ctx, GLenum pname,
				const GLfloat *param )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
	 update_global_ambient( ctx );
	 break;

      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 RADEON_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.LocalViewer)
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |= RADEON_LOCAL_VIEWER;
	 else
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_LOCAL_VIEWER;
         break;

      case GL_LIGHT_MODEL_TWO_SIDE:
	 RADEON_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.TwoSide)
	    rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= RADEON_LIGHT_TWOSIDE;
	 else
	    rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~RADEON_LIGHT_TWOSIDE;

	 check_twoside_fallback( ctx );

	 if (rmesa->radeon.TclFallback) {
	    radeonChooseRenderState( ctx );
	    radeonChooseVertexState( ctx );
	 }
         break;

      case GL_LIGHT_MODEL_COLOR_CONTROL:
	 radeonUpdateSpecular(ctx);
         break;

      default:
         break;
   }
}

static void radeonShadeModel( struct gl_context *ctx, GLenum mode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint s = rmesa->hw.set.cmd[SET_SE_CNTL];

   s &= ~(RADEON_DIFFUSE_SHADE_MASK |
	  RADEON_ALPHA_SHADE_MASK |
	  RADEON_SPECULAR_SHADE_MASK |
	  RADEON_FOG_SHADE_MASK);

   switch ( mode ) {
   case GL_FLAT:
      s |= (RADEON_DIFFUSE_SHADE_FLAT |
	    RADEON_ALPHA_SHADE_FLAT |
	    RADEON_SPECULAR_SHADE_FLAT |
	    RADEON_FOG_SHADE_FLAT);
      break;
   case GL_SMOOTH:
      s |= (RADEON_DIFFUSE_SHADE_GOURAUD |
	    RADEON_ALPHA_SHADE_GOURAUD |
	    RADEON_SPECULAR_SHADE_GOURAUD |
	    RADEON_FOG_SHADE_GOURAUD);
      break;
   default:
      return;
   }

   if ( rmesa->hw.set.cmd[SET_SE_CNTL] != s ) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = s;
   }
}


/* =============================================================
 * User clip planes
 */

static void radeonClipPlane( struct gl_context *ctx, GLenum plane, const GLfloat *eq )
{
   GLint p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

   RADEON_STATECHANGE( rmesa, ucp[p] );
   rmesa->hw.ucp[p].cmd[UCP_X] = ip[0];
   rmesa->hw.ucp[p].cmd[UCP_Y] = ip[1];
   rmesa->hw.ucp[p].cmd[UCP_Z] = ip[2];
   rmesa->hw.ucp[p].cmd[UCP_W] = ip[3];
}

static void radeonUpdateClipPlanes( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	 RADEON_STATECHANGE( rmesa, ucp[p] );
	 rmesa->hw.ucp[p].cmd[UCP_X] = ip[0];
	 rmesa->hw.ucp[p].cmd[UCP_Y] = ip[1];
	 rmesa->hw.ucp[p].cmd[UCP_Z] = ip[2];
	 rmesa->hw.ucp[p].cmd[UCP_W] = ip[3];
      }
   }
}


/* =============================================================
 * Stencil
 */

static void
radeonStencilFuncSeparate( struct gl_context *ctx, GLenum face, GLenum func,
                           GLint ref, GLuint mask )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint refmask = (((ctx->Stencil.Ref[0] & 0xff) << RADEON_STENCIL_REF_SHIFT) |
		     ((ctx->Stencil.ValueMask[0] & 0xff) << RADEON_STENCIL_MASK_SHIFT));

   RADEON_STATECHANGE( rmesa, ctx );
   RADEON_STATECHANGE( rmesa, msk );

   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~RADEON_STENCIL_TEST_MASK;
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] &= ~(RADEON_STENCIL_REF_MASK|
						   RADEON_STENCIL_VALUE_MASK);

   switch ( ctx->Stencil.Function[0] ) {
   case GL_NEVER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_TEST_ALWAYS;
      break;
   }

   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] |= refmask;
}

static void
radeonStencilMaskSeparate( struct gl_context *ctx, GLenum face, GLuint mask )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   RADEON_STATECHANGE( rmesa, msk );
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] &= ~RADEON_STENCIL_WRITE_MASK;
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] |=
      ((ctx->Stencil.WriteMask[0] & 0xff) << RADEON_STENCIL_WRITEMASK_SHIFT);
}

static void radeonStencilOpSeparate( struct gl_context *ctx, GLenum face, GLenum fail,
                                     GLenum zfail, GLenum zpass )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   /* radeon 7200 have stencil bug, DEC and INC_WRAP will actually both do DEC_WRAP,
      and DEC_WRAP (and INVERT) will do INVERT. No way to get correct INC_WRAP and DEC,
      but DEC_WRAP can be fixed by using DEC and INC_WRAP at least use INC. */

   GLuint tempRADEON_STENCIL_FAIL_DEC_WRAP;
   GLuint tempRADEON_STENCIL_FAIL_INC_WRAP;
   GLuint tempRADEON_STENCIL_ZFAIL_DEC_WRAP;
   GLuint tempRADEON_STENCIL_ZFAIL_INC_WRAP;
   GLuint tempRADEON_STENCIL_ZPASS_DEC_WRAP;
   GLuint tempRADEON_STENCIL_ZPASS_INC_WRAP;

   if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_BROKEN_STENCIL) {
      tempRADEON_STENCIL_FAIL_DEC_WRAP = RADEON_STENCIL_FAIL_DEC;
      tempRADEON_STENCIL_FAIL_INC_WRAP = RADEON_STENCIL_FAIL_INC;
      tempRADEON_STENCIL_ZFAIL_DEC_WRAP = RADEON_STENCIL_ZFAIL_DEC;
      tempRADEON_STENCIL_ZFAIL_INC_WRAP = RADEON_STENCIL_ZFAIL_INC;
      tempRADEON_STENCIL_ZPASS_DEC_WRAP = RADEON_STENCIL_ZPASS_DEC;
      tempRADEON_STENCIL_ZPASS_INC_WRAP = RADEON_STENCIL_ZPASS_INC;
   }
   else {
      tempRADEON_STENCIL_FAIL_DEC_WRAP = RADEON_STENCIL_FAIL_DEC_WRAP;
      tempRADEON_STENCIL_FAIL_INC_WRAP = RADEON_STENCIL_FAIL_INC_WRAP;
      tempRADEON_STENCIL_ZFAIL_DEC_WRAP = RADEON_STENCIL_ZFAIL_DEC_WRAP;
      tempRADEON_STENCIL_ZFAIL_INC_WRAP = RADEON_STENCIL_ZFAIL_INC_WRAP;
      tempRADEON_STENCIL_ZPASS_DEC_WRAP = RADEON_STENCIL_ZPASS_DEC_WRAP;
      tempRADEON_STENCIL_ZPASS_INC_WRAP = RADEON_STENCIL_ZPASS_INC_WRAP;
   }

   RADEON_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~(RADEON_STENCIL_FAIL_MASK |
					       RADEON_STENCIL_ZFAIL_MASK |
					       RADEON_STENCIL_ZPASS_MASK);

   switch ( ctx->Stencil.FailFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_FAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_FAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_FAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_FAIL_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_FAIL_DEC;
      break;
   case GL_INCR_WRAP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= tempRADEON_STENCIL_FAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= tempRADEON_STENCIL_FAIL_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_FAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZFailFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZFAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZFAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZFAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZFAIL_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZFAIL_DEC;
      break;
   case GL_INCR_WRAP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= tempRADEON_STENCIL_ZFAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= tempRADEON_STENCIL_ZFAIL_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZFAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZPassFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZPASS_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZPASS_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZPASS_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZPASS_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZPASS_DEC;
      break;
   case GL_INCR_WRAP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= tempRADEON_STENCIL_ZPASS_INC_WRAP;
      break;
   case GL_DECR_WRAP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= tempRADEON_STENCIL_ZPASS_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_STENCIL_ZPASS_INVERT;
      break;
   }
}



/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125


/**
 * Called when window size or position changes or viewport or depth range
 * state is changed.  We update the hardware viewport state here.
 */
void radeonUpdateWindow( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   __DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
   GLfloat xoffset = 0.0;
   GLfloat yoffset = dPriv ? (GLfloat) dPriv->h : 0;
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   const GLboolean render_to_fbo = (ctx->DrawBuffer ? _mesa_is_user_fbo(ctx->DrawBuffer) : 0);
   const GLfloat depthScale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   GLfloat y_scale, y_bias;

   if (render_to_fbo) {
      y_scale = 1.0;
      y_bias = 0;
   } else {
      y_scale = -1.0;
      y_bias = yoffset;
   }

   float_ui32_type sx = { v[MAT_SX] };
   float_ui32_type tx = { v[MAT_TX] + xoffset + SUBPIXEL_X };
   float_ui32_type sy = { v[MAT_SY] * y_scale };
   float_ui32_type ty = { (v[MAT_TY] * y_scale) + y_bias + SUBPIXEL_Y };
   float_ui32_type sz = { v[MAT_SZ] * depthScale };
   float_ui32_type tz = { v[MAT_TZ] * depthScale };

   RADEON_STATECHANGE( rmesa, vpt );

   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XSCALE]  = sx.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = tx.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YSCALE]  = sy.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = ty.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZSCALE]  = sz.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZOFFSET] = tz.ui32;
}


static void radeonViewport( struct gl_context *ctx, GLint x, GLint y,
			    GLsizei width, GLsizei height )
{
   /* Don't pipeline viewport changes, conflict with window offset
    * setting below.  Could apply deltas to rescue pipelined viewport
    * values, or keep the originals hanging around.
    */
   radeonUpdateWindow( ctx );

   radeon_viewport(ctx, x, y, width, height);
}

static void radeonDepthRange( struct gl_context *ctx, GLclampd nearval,
			      GLclampd farval )
{
   radeonUpdateWindow( ctx );
}

void radeonUpdateViewportOffset( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   __DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
   GLfloat xoffset = 0.0;
   GLfloat yoffset = (GLfloat)dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   float_ui32_type tx;
   float_ui32_type ty;

   tx.f = v[MAT_TX] + xoffset + SUBPIXEL_X;
   ty.f = (- v[MAT_TY]) + yoffset + SUBPIXEL_Y;

   if ( rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] != tx.ui32 ||
	rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] != ty.ui32 )
   {
      /* Note: this should also modify whatever data the context reset
       * code uses...
       */
      RADEON_STATECHANGE( rmesa, vpt );
      rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = tx.ui32;
      rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = ty.ui32;

      /* update polygon stipple x/y screen offset */
      {
         GLuint stx, sty;
         GLuint m = rmesa->hw.msc.cmd[MSC_RE_MISC];

         m &= ~(RADEON_STIPPLE_X_OFFSET_MASK |
                RADEON_STIPPLE_Y_OFFSET_MASK);

         /* add magic offsets, then invert */
         stx = 31 - ((-1) & RADEON_STIPPLE_COORD_MASK);
         sty = 31 - ((dPriv->h - 1)
                     & RADEON_STIPPLE_COORD_MASK);

         m |= ((stx << RADEON_STIPPLE_X_OFFSET_SHIFT) |
               (sty << RADEON_STIPPLE_Y_OFFSET_SHIFT));

         if ( rmesa->hw.msc.cmd[MSC_RE_MISC] != m ) {
            RADEON_STATECHANGE( rmesa, msc );
	    rmesa->hw.msc.cmd[MSC_RE_MISC] = m;
         }
      }
   }

   radeonUpdateScissor( ctx );
}



/* =============================================================
 * Miscellaneous
 */

static void radeonRenderMode( struct gl_context *ctx, GLenum mode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   FALLBACK( rmesa, RADEON_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}


static GLuint radeon_rop_tab[] = {
   RADEON_ROP_CLEAR,
   RADEON_ROP_AND,
   RADEON_ROP_AND_REVERSE,
   RADEON_ROP_COPY,
   RADEON_ROP_AND_INVERTED,
   RADEON_ROP_NOOP,
   RADEON_ROP_XOR,
   RADEON_ROP_OR,
   RADEON_ROP_NOR,
   RADEON_ROP_EQUIV,
   RADEON_ROP_INVERT,
   RADEON_ROP_OR_REVERSE,
   RADEON_ROP_COPY_INVERTED,
   RADEON_ROP_OR_INVERTED,
   RADEON_ROP_NAND,
   RADEON_ROP_SET,
};

static void radeonLogicOpCode( struct gl_context *ctx, GLenum opcode )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint rop = (GLuint)opcode - GL_CLEAR;

   ASSERT( rop < 16 );

   RADEON_STATECHANGE( rmesa, msk );
   rmesa->hw.msk.cmd[MSK_RB3D_ROPCNTL] = radeon_rop_tab[rop];
}

/* =============================================================
 * State enable/disable
 */

static void radeonEnable( struct gl_context *ctx, GLenum cap, GLboolean state )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint p, flag;

   if ( RADEON_DEBUG & RADEON_STATE )
      fprintf( stderr, "%s( %s = %s )\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr( cap ),
	       state ? "GL_TRUE" : "GL_FALSE" );

   switch ( cap ) {
      /* Fast track this one...
       */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      break;

   case GL_ALPHA_TEST:
      RADEON_STATECHANGE( rmesa, ctx );
      if (state) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= RADEON_ALPHA_TEST_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~RADEON_ALPHA_TEST_ENABLE;
      }
      break;

   case GL_BLEND:
      RADEON_STATECHANGE( rmesa, ctx );
      if (state) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_ALPHA_BLEND_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_ALPHA_BLEND_ENABLE;
      }
      if ( (ctx->Color.ColorLogicOpEnabled || (ctx->Color.BlendEnabled
	    && ctx->Color.Blend[0].EquationRGB == GL_LOGIC_OP)) ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_ROP_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_ROP_ENABLE;
      }

      /* Catch a possible fallback:
       */
      if (state) {
	 ctx->Driver.BlendEquationSeparate( ctx,
					    ctx->Color.Blend[0].EquationRGB,
					    ctx->Color.Blend[0].EquationA );
	 ctx->Driver.BlendFuncSeparate( ctx, ctx->Color.Blend[0].SrcRGB,
					ctx->Color.Blend[0].DstRGB,
					ctx->Color.Blend[0].SrcA,
					ctx->Color.Blend[0].DstA );
      }
      else {
	 FALLBACK( rmesa, RADEON_FALLBACK_BLEND_FUNC, GL_FALSE );
	 FALLBACK( rmesa, RADEON_FALLBACK_BLEND_EQ, GL_FALSE );
      }
      break;

   case GL_CLIP_PLANE0:
   case GL_CLIP_PLANE1:
   case GL_CLIP_PLANE2:
   case GL_CLIP_PLANE3:
   case GL_CLIP_PLANE4:
   case GL_CLIP_PLANE5:
      p = cap-GL_CLIP_PLANE0;
      RADEON_STATECHANGE( rmesa, tcl );
      if (state) {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= (RADEON_UCP_ENABLE_0<<p);
	 radeonClipPlane( ctx, cap, NULL );
      }
      else {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~(RADEON_UCP_ENABLE_0<<p);
      }
      break;

   case GL_COLOR_MATERIAL:
      radeonColorMaterial( ctx, 0, 0 );
      radeonUpdateMaterial( ctx );
      break;

   case GL_CULL_FACE:
      radeonCullFace( ctx, 0 );
      break;

   case GL_DEPTH_TEST:
      RADEON_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_Z_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_Z_ENABLE;
      }
      break;

   case GL_DITHER:
      RADEON_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_DITHER_ENABLE;
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~rmesa->radeon.state.color.roundEnable;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_DITHER_ENABLE;
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  rmesa->radeon.state.color.roundEnable;
      }
      break;

   case GL_FOG:
      RADEON_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= RADEON_FOG_ENABLE;
	 radeonFogfv( ctx, GL_FOG_MODE, NULL );
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~RADEON_FOG_ENABLE;
	 RADEON_STATECHANGE(rmesa, tcl);
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~RADEON_TCL_FOG_MASK;
      }
      radeonUpdateSpecular( ctx ); /* for PK_SPEC */
      _mesa_allow_light_in_model( ctx, !state );
      break;

   case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
      RADEON_STATECHANGE(rmesa, tcl);
      p = cap - GL_LIGHT0;
      if (p&1)
	 flag = (RADEON_LIGHT_1_ENABLE |
		 RADEON_LIGHT_1_ENABLE_AMBIENT |
		 RADEON_LIGHT_1_ENABLE_SPECULAR);
      else
	 flag = (RADEON_LIGHT_0_ENABLE |
		 RADEON_LIGHT_0_ENABLE_AMBIENT |
		 RADEON_LIGHT_0_ENABLE_SPECULAR);

      if (state)
	 rmesa->hw.tcl.cmd[p/2 + TCL_PER_LIGHT_CTL_0] |= flag;
      else
	 rmesa->hw.tcl.cmd[p/2 + TCL_PER_LIGHT_CTL_0] &= ~flag;

      /*
       */
      update_light_colors( ctx, p );
      break;

   case GL_LIGHTING:
      RADEON_STATECHANGE(rmesa, tcl);
      radeonUpdateSpecular(ctx);
      check_twoside_fallback( ctx );
      break;

   case GL_LINE_SMOOTH:
      RADEON_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  RADEON_ANTI_ALIAS_LINE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~RADEON_ANTI_ALIAS_LINE;
      }
      break;

   case GL_LINE_STIPPLE:
      RADEON_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  RADEON_PATTERN_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~RADEON_PATTERN_ENABLE;
      }
      break;

   case GL_COLOR_LOGIC_OP:
      RADEON_STATECHANGE( rmesa, ctx );
      if ( (ctx->Color.ColorLogicOpEnabled || (ctx->Color.BlendEnabled
	    && ctx->Color.Blend[0].EquationRGB == GL_LOGIC_OP)) ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_ROP_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_ROP_ENABLE;
      }
      break;

   case GL_NORMALIZE:
      RADEON_STATECHANGE( rmesa, tcl );
      if ( state ) {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |=  RADEON_NORMALIZE_NORMALS;
      } else {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_NORMALIZE_NORMALS;
      }
      break;

   case GL_POLYGON_OFFSET_POINT:
      RADEON_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  RADEON_ZBIAS_ENABLE_POINT;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~RADEON_ZBIAS_ENABLE_POINT;
      }
      break;

   case GL_POLYGON_OFFSET_LINE:
      RADEON_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  RADEON_ZBIAS_ENABLE_LINE;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~RADEON_ZBIAS_ENABLE_LINE;
      }
      break;

   case GL_POLYGON_OFFSET_FILL:
      RADEON_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  RADEON_ZBIAS_ENABLE_TRI;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~RADEON_ZBIAS_ENABLE_TRI;
      }
      break;

   case GL_POLYGON_SMOOTH:
      RADEON_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  RADEON_ANTI_ALIAS_POLY;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~RADEON_ANTI_ALIAS_POLY;
      }
      break;

   case GL_POLYGON_STIPPLE:
      RADEON_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  RADEON_STIPPLE_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~RADEON_STIPPLE_ENABLE;
      }
      break;

   case GL_RESCALE_NORMAL_EXT: {
      GLboolean tmp = ctx->_NeedEyeCoords ? state : !state;
      RADEON_STATECHANGE( rmesa, tcl );
      if ( tmp ) {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |=  RADEON_RESCALE_NORMALS;
      } else {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_RESCALE_NORMALS;
      }
      break;
   }

   case GL_SCISSOR_TEST:
      radeon_firevertices(&rmesa->radeon);
      rmesa->radeon.state.scissor.enabled = state;
      radeonUpdateScissor( ctx );
      break;

   case GL_STENCIL_TEST:
      {
	 GLboolean hw_stencil = GL_FALSE;
	 if (ctx->DrawBuffer) {
	    struct radeon_renderbuffer *rrbStencil
	       = radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
	    hw_stencil = (rrbStencil && rrbStencil->bo);
	 }

	 if (hw_stencil) {
	    RADEON_STATECHANGE( rmesa, ctx );
	    if ( state ) {
	       rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  RADEON_STENCIL_ENABLE;
	    } else {
	       rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~RADEON_STENCIL_ENABLE;
	    }
	 } else {
	    FALLBACK( rmesa, RADEON_FALLBACK_STENCIL, state );
	 }
      }
      break;

   case GL_TEXTURE_GEN_Q:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
      /* Picked up in radeonUpdateTextureState.
       */
      rmesa->recheck_texgen[ctx->Texture.CurrentUnit] = GL_TRUE;
      break;

   case GL_COLOR_SUM_EXT:
      radeonUpdateSpecular ( ctx );
      break;

   default:
      return;
   }
}


static void radeonLightingSpaceChange( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLboolean tmp;
   RADEON_STATECHANGE( rmesa, tcl );

   if (RADEON_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s %d BEFORE %x\n", __FUNCTION__, ctx->_NeedEyeCoords,
	      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL]);

   if (ctx->_NeedEyeCoords)
      tmp = ctx->Transform.RescaleNormals;
   else
      tmp = !ctx->Transform.RescaleNormals;

   if ( tmp ) {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] |=  RADEON_RESCALE_NORMALS;
   } else {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] &= ~RADEON_RESCALE_NORMALS;
   }

   if (RADEON_DEBUG & RADEON_STATE)
      fprintf(stderr, "%s %d AFTER %x\n", __FUNCTION__, ctx->_NeedEyeCoords,
	      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL]);
}

/* =============================================================
 * Deferred state management - matrices, textures, other?
 */


void radeonUploadTexMatrix( r100ContextPtr rmesa,
			    int unit, GLboolean swapcols )
{
/* Here's how this works: on r100, only 3 tex coords can be submitted, so the
   vector looks like this probably: (s t r|q 0) (not sure if the last coord
   is hardwired to 0, could be 1 too). Interestingly, it actually looks like
   texgen generates all 4 coords, at least tests with projtex indicated that.
   So: if we need the q coord in the end (solely determined by the texture
   target, i.e. 2d / 1d / texrect targets) we swap the third and 4th row.
   Additionally, if we don't have texgen but 4 tex coords submitted, we swap
   column 3 and 4 (for the 2d / 1d / texrect targets) since the q coord
   will get submitted in the "wrong", i.e. 3rd, slot.
   If an app submits 3 coords for 2d targets, we assume it is saving on vertex
   size and using the texture matrix to swap the r and q coords around (ut2k3
   does exactly that), so we don't need the 3rd / 4th column swap - still need
   the 3rd / 4th row swap of course. This will potentially break for apps which
   use TexCoord3x just for fun. Additionally, it will never work if an app uses
   an "advanced" texture matrix and relies on all 4 texcoord inputs to generate
   the maximum needed 3. This seems impossible to do with hw tcl on r100, and
   incredibly hard to detect so we can't just fallback in such a case. Assume
   it never happens... - rs
*/

   int idx = TEXMAT_0 + unit;
   float *dest = ((float *)RADEON_DB_STATE( mat[idx] )) + MAT_ELT_0;
   int i;
   struct gl_texture_unit tUnit = rmesa->radeon.glCtx->Texture.Unit[unit];
   GLfloat *src = rmesa->tmpmat[unit].m;

   rmesa->TexMatColSwap &= ~(1 << unit);
   if ((tUnit._ReallyEnabled & (TEXTURE_3D_BIT | TEXTURE_CUBE_BIT)) == 0) {
      if (swapcols) {
	 rmesa->TexMatColSwap |= 1 << unit;
	 /* attention some elems are swapped 2 times! */
	 *dest++ = src[0];
	 *dest++ = src[4];
	 *dest++ = src[12];
	 *dest++ = src[8];
	 *dest++ = src[1];
	 *dest++ = src[5];
	 *dest++ = src[13];
	 *dest++ = src[9];
	 *dest++ = src[2];
	 *dest++ = src[6];
	 *dest++ = src[15];
	 *dest++ = src[11];
	 /* those last 4 are probably never used */
	 *dest++ = src[3];
	 *dest++ = src[7];
	 *dest++ = src[14];
	 *dest++ = src[10];
      }
      else {
	 for (i = 0; i < 2; i++) {
	    *dest++ = src[i];
	    *dest++ = src[i+4];
	    *dest++ = src[i+8];
	    *dest++ = src[i+12];
	 }
	 for (i = 3; i >= 2; i--) {
	    *dest++ = src[i];
	    *dest++ = src[i+4];
	    *dest++ = src[i+8];
	    *dest++ = src[i+12];
	 }
      }
   }
   else {
      for (i = 0 ; i < 4 ; i++) {
	 *dest++ = src[i];
	 *dest++ = src[i+4];
	 *dest++ = src[i+8];
	 *dest++ = src[i+12];
      }
   }

   RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}


static void upload_matrix( r100ContextPtr rmesa, GLfloat *src, int idx )
{
   float *dest = ((float *)RADEON_DB_STATE( mat[idx] ))+MAT_ELT_0;
   int i;


   for (i = 0 ; i < 4 ; i++) {
      *dest++ = src[i];
      *dest++ = src[i+4];
      *dest++ = src[i+8];
      *dest++ = src[i+12];
   }

   RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}

static void upload_matrix_t( r100ContextPtr rmesa, GLfloat *src, int idx )
{
   float *dest = ((float *)RADEON_DB_STATE( mat[idx] ))+MAT_ELT_0;
   memcpy(dest, src, 16*sizeof(float));
   RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}


static void update_texturematrix( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT( ctx );
   GLuint tpc = rmesa->hw.tcl.cmd[TCL_TEXTURE_PROC_CTL];
   GLuint vs = rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL];
   int unit;
   GLuint texMatEnabled = 0;
   rmesa->NeedTexMatrix = 0;
   rmesa->TexMatColSwap = 0;

   for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	 GLboolean needMatrix = GL_FALSE;
	 if (ctx->TextureMatrixStack[unit].Top->type != MATRIX_IDENTITY) {
	    needMatrix = GL_TRUE;
	    texMatEnabled |= (RADEON_TEXGEN_TEXMAT_0_ENABLE |
			      RADEON_TEXMAT_0_ENABLE) << unit;

	    if (rmesa->TexGenEnabled & (RADEON_TEXMAT_0_ENABLE << unit)) {
	       /* Need to preconcatenate any active texgen
	        * obj/eyeplane matrices:
	        */
	       _math_matrix_mul_matrix( &rmesa->tmpmat[unit],
				     ctx->TextureMatrixStack[unit].Top,
				     &rmesa->TexGenMatrix[unit] );
	    }
	    else {
	       _math_matrix_copy( &rmesa->tmpmat[unit],
		  ctx->TextureMatrixStack[unit].Top );
	    }
	 }
	 else if (rmesa->TexGenEnabled & (RADEON_TEXMAT_0_ENABLE << unit)) {
	    _math_matrix_copy( &rmesa->tmpmat[unit], &rmesa->TexGenMatrix[unit] );
	    needMatrix = GL_TRUE;
	 }
	 if (needMatrix) {
	    rmesa->NeedTexMatrix |= 1 << unit;
	    radeonUploadTexMatrix( rmesa, unit,
			!ctx->Texture.Unit[unit].TexGenEnabled );
	 }
      }
   }

   tpc = (texMatEnabled | rmesa->TexGenEnabled);

   /* TCL_TEX_COMPUTED_x is TCL_TEX_INPUT_x | 0x8 */
   vs &= ~((RADEON_TCL_TEX_COMPUTED_TEX_0 << RADEON_TCL_TEX_0_OUTPUT_SHIFT) |
	   (RADEON_TCL_TEX_COMPUTED_TEX_0 << RADEON_TCL_TEX_1_OUTPUT_SHIFT) |
	   (RADEON_TCL_TEX_COMPUTED_TEX_0 << RADEON_TCL_TEX_2_OUTPUT_SHIFT));

   vs |= (((tpc & RADEON_TEXGEN_TEXMAT_0_ENABLE) <<
	 (RADEON_TCL_TEX_0_OUTPUT_SHIFT + 3)) |
      ((tpc & RADEON_TEXGEN_TEXMAT_1_ENABLE) <<
	 (RADEON_TCL_TEX_1_OUTPUT_SHIFT + 2)) |
      ((tpc & RADEON_TEXGEN_TEXMAT_2_ENABLE) <<
	 (RADEON_TCL_TEX_2_OUTPUT_SHIFT + 1)));

   if (tpc != rmesa->hw.tcl.cmd[TCL_TEXTURE_PROC_CTL] ||
       vs != rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL]) {

      RADEON_STATECHANGE(rmesa, tcl);
      rmesa->hw.tcl.cmd[TCL_TEXTURE_PROC_CTL] = tpc;
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] = vs;
   }
}

static GLboolean r100ValidateBuffers(struct gl_context *ctx)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb;
   int i, ret;

   radeon_cs_space_reset_bos(rmesa->radeon.cmdbuf.cs);

   rrb = radeon_get_colorbuffer(&rmesa->radeon);
   /* color buffer */
   if (rrb && rrb->bo) {
     radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, rrb->bo,
				       0, RADEON_GEM_DOMAIN_VRAM);
   }

   /* depth buffer */
   rrb = radeon_get_depthbuffer(&rmesa->radeon);
   /* color buffer */
   if (rrb && rrb->bo) {
     radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, rrb->bo,
				       0, RADEON_GEM_DOMAIN_VRAM);
   }

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; ++i) {
      radeonTexObj *t;

      if (!ctx->Texture.Unit[i]._ReallyEnabled)
	 continue;

      t = rmesa->state.texture.unit[i].texobj;

      if (!t)
	 continue;
      if (t->image_override && t->bo)
	radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, t->bo,
			   RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0);
      else if (t->mt->bo)
	radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, t->mt->bo,
			   RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0);
   }

   ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs, first_elem(&rmesa->radeon.dma.reserved)->bo, RADEON_GEM_DOMAIN_GTT, 0);
   if (ret)
       return GL_FALSE;
   return GL_TRUE;
}

GLboolean radeonValidateState( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint new_state = rmesa->radeon.NewGLState;

   if (new_state & _NEW_BUFFERS) {
     _mesa_update_framebuffer(ctx);
     /* this updates the DrawBuffer's Width/Height if it's a FBO */
     _mesa_update_draw_buffer_bounds(ctx);
     RADEON_STATECHANGE(rmesa, ctx);
   }

   if (new_state & _NEW_TEXTURE) {
      radeonUpdateTextureState( ctx );
      new_state |= rmesa->radeon.NewGLState; /* may add TEXTURE_MATRIX */
   }

   /* we need to do a space check here */
   if (!r100ValidateBuffers(ctx))
     return GL_FALSE;

   /* Need an event driven matrix update?
    */
   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
      upload_matrix( rmesa, ctx->_ModelProjectMatrix.m, MODEL_PROJ );

   /* Need these for lighting (shouldn't upload otherwise)
    */
   if (new_state & (_NEW_MODELVIEW)) {
      upload_matrix( rmesa, ctx->ModelviewMatrixStack.Top->m, MODEL );
      upload_matrix_t( rmesa, ctx->ModelviewMatrixStack.Top->inv, MODEL_IT );
   }

   /* Does this need to be triggered on eg. modelview for
    * texgen-derived objplane/eyeplane matrices?
    */
   if (new_state & _NEW_TEXTURE_MATRIX) {
      update_texturematrix( ctx );
   }

   if (new_state & (_NEW_LIGHT|_NEW_MODELVIEW|_MESA_NEW_NEED_EYE_COORDS)) {
      update_light( ctx );
   }

   /* emit all active clip planes if projection matrix changes.
    */
   if (new_state & (_NEW_PROJECTION)) {
      if (ctx->Transform.ClipPlanesEnabled)
	 radeonUpdateClipPlanes( ctx );
   }


   rmesa->radeon.NewGLState = 0;

   return GL_TRUE;
}


static void radeonInvalidateState( struct gl_context *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _ae_invalidate_state( ctx, new_state );
   R100_CONTEXT(ctx)->radeon.NewGLState |= new_state;
}


/* A hack.  Need a faster way to find this out.
 */
static GLboolean check_material( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLint i;

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT;
	i < _TNL_ATTRIB_MAT_BACK_INDEXES;
	i++)
      if (tnl->vb.AttribPtr[i] &&
	  tnl->vb.AttribPtr[i]->stride)
	 return GL_TRUE;

   return GL_FALSE;
}


static void radeonWrapRunPipeline( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLboolean has_material;

   if (0)
      fprintf(stderr, "%s, newstate: %x\n", __FUNCTION__, rmesa->radeon.NewGLState);

   /* Validate state:
    */
   if (rmesa->radeon.NewGLState)
      if (!radeonValidateState( ctx ))
	 FALLBACK(rmesa, RADEON_FALLBACK_TEXTURE, GL_TRUE);

   has_material = (ctx->Light.Enabled && check_material( ctx ));

   if (has_material) {
      TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_MATERIAL, GL_TRUE );
   }

   /* Run the pipeline.
    */
   _tnl_run_pipeline( ctx );

   if (has_material) {
      TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_MATERIAL, GL_FALSE );
   }
}

static void radeonPolygonStipple( struct gl_context *ctx, const GLubyte *mask )
{
   r100ContextPtr r100 = R100_CONTEXT(ctx);
   GLint i;

   radeon_firevertices(&r100->radeon);

   RADEON_STATECHANGE(r100, stp);

   /* Must flip pattern upside down.
    */
   for ( i = 31 ; i >= 0; i--) {
     r100->hw.stp.cmd[3 + i] = ((GLuint *) mask)[i];
   }
}


/* Initialize the driver's state functions.
 * Many of the ctx->Driver functions might have been initialized to
 * software defaults in the earlier _mesa_init_driver_functions() call.
 */
void radeonInitStateFuncs( struct gl_context *ctx )
{
   ctx->Driver.UpdateState		= radeonInvalidateState;
   ctx->Driver.LightingSpaceChange      = radeonLightingSpaceChange;

   ctx->Driver.DrawBuffer		= radeonDrawBuffer;
   ctx->Driver.ReadBuffer		= radeonReadBuffer;
   ctx->Driver.CopyPixels               = _mesa_meta_CopyPixels;
   ctx->Driver.DrawPixels               = _mesa_meta_DrawPixels;
   ctx->Driver.ReadPixels               = radeonReadPixels;

   ctx->Driver.AlphaFunc		= radeonAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= radeonBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= radeonBlendFuncSeparate;
   ctx->Driver.ClipPlane		= radeonClipPlane;
   ctx->Driver.ColorMask		= radeonColorMask;
   ctx->Driver.CullFace			= radeonCullFace;
   ctx->Driver.DepthFunc		= radeonDepthFunc;
   ctx->Driver.DepthMask		= radeonDepthMask;
   ctx->Driver.DepthRange		= radeonDepthRange;
   ctx->Driver.Enable			= radeonEnable;
   ctx->Driver.Fogfv			= radeonFogfv;
   ctx->Driver.FrontFace		= radeonFrontFace;
   ctx->Driver.Hint			= NULL;
   ctx->Driver.LightModelfv		= radeonLightModelfv;
   ctx->Driver.Lightfv			= radeonLightfv;
   ctx->Driver.LineStipple              = radeonLineStipple;
   ctx->Driver.LineWidth                = radeonLineWidth;
   ctx->Driver.LogicOpcode		= radeonLogicOpCode;
   ctx->Driver.PolygonMode		= radeonPolygonMode;
   ctx->Driver.PolygonOffset		= radeonPolygonOffset;
   ctx->Driver.PolygonStipple		= radeonPolygonStipple;
   ctx->Driver.RenderMode		= radeonRenderMode;
   ctx->Driver.Scissor			= radeonScissor;
   ctx->Driver.ShadeModel		= radeonShadeModel;
   ctx->Driver.StencilFuncSeparate	= radeonStencilFuncSeparate;
   ctx->Driver.StencilMaskSeparate	= radeonStencilMaskSeparate;
   ctx->Driver.StencilOpSeparate	= radeonStencilOpSeparate;
   ctx->Driver.Viewport			= radeonViewport;

   TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange = radeonUpdateMaterial;
   TNL_CONTEXT(ctx)->Driver.RunPipeline = radeonWrapRunPipeline;
}
