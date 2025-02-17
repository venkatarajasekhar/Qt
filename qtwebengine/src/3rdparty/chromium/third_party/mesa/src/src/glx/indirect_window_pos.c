/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <GL/gl.h>
#include "indirect.h"

void
__indirect_glWindowPos2dMESA(GLdouble x, GLdouble y)
{
   __indirect_glWindowPos3fMESA(x, y, 0.0);
}

void
__indirect_glWindowPos2iMESA(GLint x, GLint y)
{
   __indirect_glWindowPos3fMESA(x, y, 0.0);
}

void
__indirect_glWindowPos2fMESA(GLfloat x, GLfloat y)
{
   __indirect_glWindowPos3fMESA(x, y, 0.0);
}

void
__indirect_glWindowPos2sMESA(GLshort x, GLshort y)
{
   __indirect_glWindowPos3fMESA(x, y, 0.0);
}

void
__indirect_glWindowPos2dvMESA(const GLdouble * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], 0.0);
}

void
__indirect_glWindowPos2fvMESA(const GLfloat * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], 0.0);
}

void
__indirect_glWindowPos2ivMESA(const GLint * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], 0.0);
}

void
__indirect_glWindowPos2svMESA(const GLshort * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], 0.0);
}

void
__indirect_glWindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   __indirect_glWindowPos3fMESA(x, y, z);
}

void
__indirect_glWindowPos3iMESA(GLint x, GLint y, GLint z)
{
   __indirect_glWindowPos3fMESA(x, y, z);
}

void
__indirect_glWindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   __indirect_glWindowPos3fMESA(x, y, z);
}

void
__indirect_glWindowPos3dvMESA(const GLdouble * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], p[2]);
}

void
__indirect_glWindowPos3ivMESA(const GLint * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], p[2]);
}

void
__indirect_glWindowPos3svMESA(const GLshort * p)
{
   __indirect_glWindowPos3fMESA(p[0], p[1], p[2]);
}
