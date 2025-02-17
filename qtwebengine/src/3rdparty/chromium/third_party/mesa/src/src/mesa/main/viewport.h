/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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


#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "glheader.h"

struct gl_context;

extern void GLAPIENTRY
_mesa_Viewport(GLint x, GLint y, GLsizei width, GLsizei height);


extern void 
_mesa_set_viewport(struct gl_context *ctx, GLint x, GLint y,
                   GLsizei width, GLsizei height);


extern void GLAPIENTRY
_mesa_DepthRange(GLclampd nearval, GLclampd farval);

extern void GLAPIENTRY
_mesa_DepthRangef(GLclampf nearval, GLclampf farval);


extern void 
_mesa_init_viewport(struct gl_context *ctx);


extern void 
_mesa_free_viewport_data(struct gl_context *ctx);


#endif
