/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __R200_TEX_H__
#define __R200_TEX_H__

extern void r200SetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv);
extern void r200SetTexBuffer2(__DRIcontext *pDRICtx, GLint target, GLint glx_texture_format,
			      __DRIdrawable *dPriv);

extern void r200UpdateTextureState( struct gl_context *ctx );

extern int r200UploadTexImages( r200ContextPtr rmesa, radeonTexObjPtr t, GLuint face );

extern void r200DestroyTexObj( r200ContextPtr rmesa, radeonTexObjPtr t );

extern void r200InitTextureFuncs( radeonContextPtr radeon, struct dd_function_table *functions );

extern void r200UpdateFragmentShader( struct gl_context *ctx );
extern void r200TexUpdateParameters(struct gl_context *ctx, GLuint unit);

extern void set_re_cntl_d3d( struct gl_context *ctx, int unit, GLboolean use_d3d );

#endif /* __R200_TEX_H__ */
