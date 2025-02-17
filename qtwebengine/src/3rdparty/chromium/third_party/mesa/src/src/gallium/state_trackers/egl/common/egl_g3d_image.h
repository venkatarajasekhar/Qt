/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef _EGL_G3D_IMAGE_H_
#define _EGL_G3D_IMAGE_H_

#include "egl_g3d.h"

_EGLImage *
egl_g3d_create_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx,
                     EGLenum target, EGLClientBuffer buffer,
                     const EGLint *attribs);

EGLBoolean
egl_g3d_destroy_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *image);

_EGLImage *
egl_g3d_create_drm_image(_EGLDriver *drv, _EGLDisplay *dpy,
                         const EGLint *attribs);

EGLBoolean
egl_g3d_export_drm_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img,
			 EGLint *name, EGLint *handle, EGLint *stride);

#endif /* _EGL_G3D_IMAGE_H_ */
