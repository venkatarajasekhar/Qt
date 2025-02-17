/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/macros.h"

#include "s_context.h"
#include "s_logic.h"
#include "s_span.h"


/**
 * We do all logic ops on 4-byte GLuints.
 * Depending on bytes per pixel, the mask array elements correspond to
 * 1, 2 or 4 GLuints.
 */
#define LOGIC_OP_LOOP(MODE, MASKSTRIDE)		\
do {						\
   GLuint i;					\
   switch (MODE) {				\
      case GL_CLEAR:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = 0;			\
	    }					\
	 }					\
	 break;					\
      case GL_SET:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~0;			\
	    }					\
	 }					\
	 break;					\
      case GL_COPY:				\
	 /* do nothing */			\
	 break;					\
      case GL_COPY_INVERTED:			\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~src[i];		\
	    }					\
	 }					\
	 break;					\
      case GL_NOOP:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = dest[i];		\
	    }					\
	 }					\
	 break;					\
      case GL_INVERT:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~dest[i];		\
	    }					\
	 }					\
	 break;					\
      case GL_AND:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] &= dest[i];		\
	    }					\
	 }					\
	 break;					\
      case GL_NAND:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~(src[i] & dest[i]);	\
	    }					\
	 }					\
	 break;					\
      case GL_OR:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] |= dest[i];		\
	    }					\
	 }					\
	 break;					\
      case GL_NOR:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~(src[i] | dest[i]);	\
	    }					\
	 }					\
	 break;					\
      case GL_XOR:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] ^= dest[i];		\
	    }					\
	 }					\
	 break;					\
      case GL_EQUIV:				\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~(src[i] ^ dest[i]);	\
	    }					\
	 }					\
	 break;					\
      case GL_AND_REVERSE:			\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = src[i] & ~dest[i];	\
	    }					\
	 }					\
	 break;					\
      case GL_AND_INVERTED:			\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~src[i] & dest[i];	\
	    }					\
	 }					\
	 break;					\
      case GL_OR_REVERSE:			\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = src[i] | ~dest[i];	\
	    }					\
	 }					\
	 break;					\
      case GL_OR_INVERTED:			\
         for (i = 0; i < n; i++) {		\
	    if (mask[i / MASKSTRIDE]) {		\
	       src[i] = ~src[i] | dest[i];	\
	    }					\
	 }					\
	 break;					\
      default:					\
	 _mesa_problem(ctx, "bad logicop mode");\
   }						\
} while (0)



static inline void
logicop_uint1(struct gl_context *ctx, GLuint n, GLuint src[], const GLuint dest[],
              const GLubyte mask[])
{
   LOGIC_OP_LOOP(ctx->Color.LogicOp, 1);
}


static inline void
logicop_uint2(struct gl_context *ctx, GLuint n, GLuint src[], const GLuint dest[],
              const GLubyte mask[])
{
   LOGIC_OP_LOOP(ctx->Color.LogicOp, 2);
}


static inline void
logicop_uint4(struct gl_context *ctx, GLuint n, GLuint src[], const GLuint dest[],
              const GLubyte mask[])
{
   LOGIC_OP_LOOP(ctx->Color.LogicOp, 4);
}



/**
 * Apply the current logic operator to a span of RGBA pixels.
 * We can handle horizontal runs of pixels (spans) or arrays of x/y
 * pixel coordinates.
 */
void
_swrast_logicop_rgba_span(struct gl_context *ctx, struct gl_renderbuffer *rb,
                          SWspan *span)
{
   void *rbPixels;

   ASSERT(span->end < SWRAST_MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_RGBA);

   rbPixels = _swrast_get_dest_rgba(ctx, rb, span);

   if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      /* treat 4*GLubyte as GLuint */
      logicop_uint1(ctx, span->end,
                    (GLuint *) span->array->rgba8,
                    (const GLuint *) rbPixels, span->array->mask);
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      /* treat 2*GLushort as GLuint */
      logicop_uint2(ctx, 2 * span->end,
                    (GLuint *) span->array->rgba16,
                    (const GLuint *) rbPixels, span->array->mask);
   }
   else {
      logicop_uint4(ctx, 4 * span->end,
                    (GLuint *) span->array->attribs[FRAG_ATTRIB_COL0],
                    (const GLuint *) rbPixels, span->array->mask);
   }
}
