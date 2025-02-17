/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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
 **********************************************************/

#ifndef SVGA_TGSI_EMIT_H
#define SVGA_TGSI_EMIT_H

#include "tgsi/tgsi_scan.h"
#include "svga_hw_reg.h"
#include "svga_tgsi.h"
#include "svga3d_shaderdefs.h"

struct src_register
{
   SVGA3dShaderSrcToken base;
   SVGA3dShaderSrcToken indirect;
};


struct svga_arl_consts {
   int number;
   int idx;
   int swizzle;
   int arl_num;
};

/* Internal functions:
 */

struct svga_shader_emitter
{
   unsigned size;
   char *buf;
   char *ptr;

   struct svga_compile_key key;
   struct tgsi_shader_info info;
   int unit;

   int imm_start;

   int nr_hw_float_const;
   int nr_hw_int_const;
   int nr_hw_temp;
   
   int insn_offset;

   int internal_temp_count;
   int internal_imm_count;

   int internal_color_idx[2]; /* diffuse, specular */
   int internal_color_count;

   boolean emitted_vface;
   boolean emit_frontface;
   int internal_frontface_idx;

   int ps30_input_count;
   int vs30_output_count;

   int dynamic_branching_level;

   boolean in_main_func;

   boolean created_zero_immediate;
   int zero_immediate_idx;

   boolean created_loop_const;
   int loop_const_idx;

   unsigned inverted_texcoords;  /**< bitmask of which texcoords are flipped */
   struct src_register ps_true_texcoord[PIPE_MAX_ATTRIBS];
   struct src_register ps_inverted_texcoord[PIPE_MAX_ATTRIBS];
   unsigned ps_inverted_texcoord_input[PIPE_MAX_ATTRIBS];

   unsigned label[32];
   unsigned nr_labels;

   struct src_register input_map[PIPE_MAX_ATTRIBS];
   SVGA3dShaderDestToken output_map[PIPE_MAX_ATTRIBS];

   boolean ps_reads_pos;
   boolean emitted_depth_fog;
   struct src_register ps_true_pos;
   struct src_register ps_depth_pos;
   SVGA3dShaderDestToken ps_temp_pos;

   /* shared input for depth and fog */
   struct src_register ps_depth_fog;

   struct src_register imm_0055;
   SVGA3dShaderDestToken temp_pos;
   SVGA3dShaderDestToken true_pos;
   SVGA3dShaderDestToken depth_pos;

   /* shared output for depth and fog */
   SVGA3dShaderDestToken vs_depth_fog;

   SVGA3dShaderDestToken temp_col[PIPE_MAX_COLOR_BUFS];
   SVGA3dShaderDestToken true_col[PIPE_MAX_COLOR_BUFS];

   SVGA3dShaderDestToken temp_psiz;
   SVGA3dShaderDestToken true_psiz;

   struct svga_arl_consts arl_consts[12];
   int num_arl_consts;
   int current_arl;
};


boolean svga_shader_emit_dword( struct svga_shader_emitter *emit,
                                unsigned dword );

boolean svga_shader_emit_dwords( struct svga_shader_emitter *emit,
                                 const unsigned *dwords,
                                 unsigned nr );

boolean svga_shader_emit_opcode( struct svga_shader_emitter *emit,
                                 unsigned opcode );

boolean svga_shader_emit_instructions( struct svga_shader_emitter *emit,
                                       const struct tgsi_token *tokens );

boolean svga_translate_decl_sm30( struct svga_shader_emitter *emit,
                               const struct tgsi_full_declaration *decl );


static INLINE boolean emit_dst( struct svga_shader_emitter *emit,
                         SVGA3dShaderDestToken dest )
{
   assert(dest.reserved0);
   assert(dest.mask);
   return svga_shader_emit_dword( emit, dest.value );
}

static INLINE boolean emit_src( struct svga_shader_emitter *emit,
                         const struct src_register src )
{
   if (src.base.relAddr) {
      assert(src.base.reserved0);
      assert(src.indirect.reserved0);
      return (svga_shader_emit_dword( emit, src.base.value ) &&
              svga_shader_emit_dword( emit, src.indirect.value ));
   }
   else {
      assert(src.base.reserved0);
      return svga_shader_emit_dword( emit, src.base.value );
   }
}


static INLINE boolean emit_instruction( struct svga_shader_emitter *emit,
                                 SVGA3dShaderInstToken opcode )
{
   return svga_shader_emit_opcode( emit, opcode.value );
}


static INLINE boolean emit_op1( struct svga_shader_emitter *emit,
                         SVGA3dShaderInstToken inst,
                         SVGA3dShaderDestToken dest,
                         struct src_register src0 )
{
   return (emit_instruction( emit, inst ) &&
           emit_dst( emit, dest ) &&
           emit_src( emit, src0 ));
}

static INLINE boolean emit_op2( struct svga_shader_emitter *emit,
                     SVGA3dShaderInstToken inst,
                     SVGA3dShaderDestToken dest,
                     struct src_register src0,
                     struct src_register src1 )
{
   return (emit_instruction( emit, inst ) &&
           emit_dst( emit, dest ) &&
           emit_src( emit, src0 ) &&
           emit_src( emit, src1 ));
}

static INLINE boolean emit_op3( struct svga_shader_emitter *emit,
                         SVGA3dShaderInstToken inst,
                         SVGA3dShaderDestToken dest,
                         struct src_register src0,
                         struct src_register src1,
                         struct src_register src2 )
{
   return (emit_instruction( emit, inst ) &&
           emit_dst( emit, dest ) &&
           emit_src( emit, src0 ) &&
           emit_src( emit, src1 ) &&
           emit_src( emit, src2 ));
}


static INLINE boolean emit_op4( struct svga_shader_emitter *emit,
                                SVGA3dShaderInstToken inst,
                                SVGA3dShaderDestToken dest,
                                struct src_register src0,
                                struct src_register src1,
                                struct src_register src2,
                                struct src_register src3)
{
   return (emit_instruction( emit, inst ) &&
           emit_dst( emit, dest ) &&
           emit_src( emit, src0 ) &&
           emit_src( emit, src1 ) &&
           emit_src( emit, src2 ) &&
           emit_src( emit, src3 ));
}


#define TRANSLATE_SWIZZLE(x,y,z,w)  ((x) | ((y) << 2) | ((z) << 4) | ((w) << 6))
#define SWIZZLE_XYZW  \
 TRANSLATE_SWIZZLE(TGSI_SWIZZLE_X,TGSI_SWIZZLE_Y,TGSI_SWIZZLE_Z,TGSI_SWIZZLE_W)
#define SWIZZLE_XXXX  \
 TRANSLATE_SWIZZLE(TGSI_SWIZZLE_X,TGSI_SWIZZLE_X,TGSI_SWIZZLE_X,TGSI_SWIZZLE_X)
#define SWIZZLE_YYYY  \
 TRANSLATE_SWIZZLE(TGSI_SWIZZLE_Y,TGSI_SWIZZLE_Y,TGSI_SWIZZLE_Y,TGSI_SWIZZLE_Y)
#define SWIZZLE_ZZZZ  \
 TRANSLATE_SWIZZLE(TGSI_SWIZZLE_Z,TGSI_SWIZZLE_Z,TGSI_SWIZZLE_Z,TGSI_SWIZZLE_Z)
#define SWIZZLE_WWWW  \
 TRANSLATE_SWIZZLE(TGSI_SWIZZLE_W,TGSI_SWIZZLE_W,TGSI_SWIZZLE_W,TGSI_SWIZZLE_W)



static INLINE SVGA3dShaderInstToken
inst_token( unsigned opcode )
{
   SVGA3dShaderInstToken inst;

   inst.value = 0;
   inst.op = opcode;

   return inst;
}


/**
 * Create an instance of a SVGA3dShaderDestToken.
 * Note that this function is used to create tokens for output registers,
 * temp registers AND constants (see emit_def_const()).
 */
static INLINE SVGA3dShaderDestToken 
dst_register( unsigned file,
              int number )
{
   SVGA3dShaderDestToken dest;

   /* check values against bitfield sizes */
   assert(number < (1 << 11));
   assert(file <= SVGA3DREG_PREDICATE);

   dest.value = 0;
   dest.num = number;
   dest.type_upper = file >> 3;
   dest.relAddr = 0;
   dest.reserved1 = 0;
   dest.mask = 0xf;
   dest.dstMod = 0;
   dest.shfScale = 0;
   dest.type_lower = file & 0x7;
   dest.reserved0 = 1;          /* is_reg */
   
   return dest;
}

static INLINE SVGA3dShaderDestToken
writemask( SVGA3dShaderDestToken dest,
           unsigned mask )
{
   assert(dest.mask & mask);
   dest.mask &= mask;
   return dest;
}


static INLINE SVGA3dShaderSrcToken 
src_token( unsigned file, int number )
{
   SVGA3dShaderSrcToken src;

   /* check values against bitfield sizes */
   assert(number < (1 << 11));
   assert(file <= SVGA3DREG_PREDICATE);

   src.value = 0;
   src.num = number;
   src.type_upper = file >> 3;
   src.relAddr = 0;
   src.reserved1 = 0;
   src.swizzle = SWIZZLE_XYZW;
   src.srcMod = 0;
   src.type_lower = file & 0x7;
   src.reserved0 = 1;           /* is_reg */

   return src;
}


static INLINE struct src_register 
absolute( struct src_register src )
{
   src.base.srcMod = SVGA3DSRCMOD_ABS;

   return src;
}


static INLINE struct src_register 
negate( struct src_register src )
{
   switch (src.base.srcMod) {
   case SVGA3DSRCMOD_ABS:
      src.base.srcMod = SVGA3DSRCMOD_ABSNEG;
      break;
   case SVGA3DSRCMOD_ABSNEG:
      src.base.srcMod = SVGA3DSRCMOD_ABS;
      break;
   case SVGA3DSRCMOD_NEG:
      src.base.srcMod = SVGA3DSRCMOD_NONE;
      break;
   case SVGA3DSRCMOD_NONE:
      src.base.srcMod = SVGA3DSRCMOD_NEG;
      break;
   }
   return src;
}


static INLINE struct src_register 
src_register( unsigned file, int number )
{
   struct src_register src;
   
   src.base = src_token( file, number );
   src.indirect.value = 0;

   return src;
}

static INLINE SVGA3dShaderDestToken dst( struct src_register src )
{
   return dst_register( SVGA3dShaderGetRegType( src.base.value ),
                        src.base.num );
}

static INLINE struct src_register src( SVGA3dShaderDestToken dst )
{
   return src_register( SVGA3dShaderGetRegType( dst.value ),
                        dst.num );
}

static INLINE ubyte svga_tgsi_sampler_type( struct svga_shader_emitter *emit,
                                            int idx )
{
   switch (emit->key.fkey.tex[idx].texture_target) {
   case PIPE_TEXTURE_1D:
      return SVGA3DSAMP_2D;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      return SVGA3DSAMP_2D;
   case PIPE_TEXTURE_3D:
      return SVGA3DSAMP_VOLUME;
   case PIPE_TEXTURE_CUBE:
      return SVGA3DSAMP_CUBE;
   }

   return SVGA3DSAMP_UNKNOWN;
}

#endif
