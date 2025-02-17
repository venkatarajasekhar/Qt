/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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

/**
 * \file xm_api.c
 *
 * All the XMesa* API functions.
 *
 *
 * NOTES:
 *
 * The window coordinate system origin (0,0) is in the lower-left corner
 * of the window.  X11's window coordinate origin is in the upper-left
 * corner of the window.  Therefore, most drawing functions in this
 * file have to flip Y coordinates.
 *
 *
 * Byte swapping:  If the Mesa host and the X display use a different
 * byte order then there's some trickiness to be aware of when using
 * XImages.  The byte ordering used for the XImage is that of the X
 * display, not the Mesa host.
 * The color-to-pixel encoding for True/DirectColor must be done
 * according to the display's visual red_mask, green_mask, and blue_mask.
 * If XPutPixel is used to put a pixel into an XImage then XPutPixel will
 * do byte swapping if needed.  If one wants to directly "poke" the pixel
 * into the XImage's buffer then the pixel must be byte swapped first.
 *
 */

#ifdef __CYGWIN__
#undef WIN32
#undef __WIN32__
#endif

#include "xm_api.h"
#include "xm_st.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_atomic.h"
#include "util/u_inlines.h"

#include "xm_public.h"
#include <GL/glx.h>


/* Driver interface routines, set up by xlib backend on library
 * _init().  These are global in the same way that function names are
 * global.
 */
static struct xm_driver driver;
static struct st_api *stapi;

/* Default strict invalidate to false.  This means we will not call
 * XGetGeometry after every swapbuffers, which allows swapbuffers to
 * remain asynchronous.  For apps running at 100fps with synchronous
 * swapping, a 10% boost is typical.  For gears, I see closer to 20%
 * speedup.
 *
 * Note that the work of copying data on swapbuffers doesn't disappear
 * - this change just allows the X server to execute the PutImage
 * asynchronously without us effectively blocked until its completion.
 *
 * This speeds up even llvmpipe's threaded rasterization as the
 * swapbuffers operation was a large part of the serial component of
 * an llvmpipe frame.
 *
 * The downside of this is correctness - applications which don't call
 * glViewport on window resizes will get incorrect rendering.  A
 * better solution would be to have per-frame but asynchronous
 * invalidation.  Xcb almost looks as if it could provide this, but
 * the API doesn't seem to quite be there.
 */
boolean xmesa_strict_invalidate = FALSE;

void xmesa_set_driver( const struct xm_driver *templ )
{
   driver = *templ;
   stapi = driver.create_st_api();

   xmesa_strict_invalidate =
      debug_get_bool_option("XMESA_STRICT_INVALIDATE", FALSE);
}


/*
 * XXX replace this with a linked list, or better yet, try to attach the
 * gallium/mesa extra bits to the X Display object with XAddExtension().
 */
#define MAX_DISPLAYS 10
static struct xmesa_display Displays[MAX_DISPLAYS];
static int NumDisplays = 0;

static int
xmesa_get_param(struct st_manager *smapi,
                enum st_manager_param param)
{
   switch(param) {
   case ST_MANAGER_BROKEN_INVALIDATE:
      return !xmesa_strict_invalidate;
   default:
      return 0;
   }
}

static XMesaDisplay
xmesa_init_display( Display *display )
{
   pipe_static_mutex(init_mutex);
   XMesaDisplay xmdpy;
   int i;

   pipe_mutex_lock(init_mutex);

   /* Look for XMesaDisplay which corresponds to 'display' */
   for (i = 0; i < NumDisplays; i++) {
      if (Displays[i].display == display) {
         /* Found it */
         pipe_mutex_unlock(init_mutex);
         return &Displays[i];
      }
   }

   /* Create new XMesaDisplay */

   assert(NumDisplays < MAX_DISPLAYS);
   xmdpy = &Displays[NumDisplays];
   NumDisplays++;

   if (!xmdpy->display && display) {
      xmdpy->display = display;
      xmdpy->screen = driver.create_pipe_screen(display);
      xmdpy->smapi = CALLOC_STRUCT(st_manager);
      if (xmdpy->smapi) {
         xmdpy->smapi->screen = xmdpy->screen;
         xmdpy->smapi->get_param = xmesa_get_param;
      }

      if (xmdpy->screen && xmdpy->smapi) {
         pipe_mutex_init(xmdpy->mutex);
      }
      else {
         if (xmdpy->screen) {
            xmdpy->screen->destroy(xmdpy->screen);
            xmdpy->screen = NULL;
         }
         if (xmdpy->smapi) {
            FREE(xmdpy->smapi);
            xmdpy->smapi = NULL;
         }

         xmdpy->display = NULL;
      }
   }
   if (!xmdpy->display || xmdpy->display != display)
      xmdpy = NULL;

   pipe_mutex_unlock(init_mutex);

   return xmdpy;
}

/**********************************************************************/
/*****                     X Utility Functions                    *****/
/**********************************************************************/


/**
 * Return the host's byte order as LSBFirst or MSBFirst ala X.
 */
static int host_byte_order( void )
{
   int i = 1;
   char *cptr = (char *) &i;
   return (*cptr==1) ? LSBFirst : MSBFirst;
}




/**
 * Return the true number of bits per pixel for XImages.
 * For example, if we request a 24-bit deep visual we may actually need/get
 * 32bpp XImages.  This function returns the appropriate bpp.
 * Input:  dpy - the X display
 *         visinfo - desribes the visual to be used for XImages
 * Return:  true number of bits per pixel for XImages
 */
static int
bits_per_pixel( XMesaVisual xmv )
{
   Display *dpy = xmv->display;
   XVisualInfo * visinfo = xmv->visinfo;
   XImage *img;
   int bitsPerPixel;
   /* Create a temporary XImage */
   img = XCreateImage( dpy, visinfo->visual, visinfo->depth,
		       ZPixmap, 0,           /*format, offset*/
		       (char*) MALLOC(8),    /*data*/
		       1, 1,                 /*width, height*/
		       32,                   /*bitmap_pad*/
		       0                     /*bytes_per_line*/
                     );
   assert(img);
   /* grab the bits/pixel value */
   bitsPerPixel = img->bits_per_pixel;
   /* free the XImage */
   free( img->data );
   img->data = NULL;
   XDestroyImage( img );
   return bitsPerPixel;
}



/*
 * Determine if a given X window ID is valid (window exists).
 * Do this by calling XGetWindowAttributes() for the window and
 * checking if we catch an X error.
 * Input:  dpy - the display
 *         win - the window to check for existance
 * Return:  GL_TRUE - window exists
 *          GL_FALSE - window doesn't exist
 */
static GLboolean WindowExistsFlag;

static int window_exists_err_handler( Display* dpy, XErrorEvent* xerr )
{
   (void) dpy;
   if (xerr->error_code == BadWindow) {
      WindowExistsFlag = GL_FALSE;
   }
   return 0;
}

static GLboolean window_exists( Display *dpy, Window win )
{
   XWindowAttributes wa;
   int (*old_handler)( Display*, XErrorEvent* );
   WindowExistsFlag = GL_TRUE;
   old_handler = XSetErrorHandler(window_exists_err_handler);
   XGetWindowAttributes( dpy, win, &wa ); /* dummy request */
   XSetErrorHandler(old_handler);
   return WindowExistsFlag;
}

static Status
get_drawable_size( Display *dpy, Drawable d, uint *width, uint *height )
{
   Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
   return stat;
}


/**
 * Return the size of the window (or pixmap) that corresponds to the
 * given XMesaBuffer.
 * \param width  returns width in pixels
 * \param height  returns height in pixels
 */
void
xmesa_get_window_size(Display *dpy, XMesaBuffer b,
                      GLuint *width, GLuint *height)
{
   XMesaDisplay xmdpy = xmesa_init_display(dpy);
   Status stat;

   pipe_mutex_lock(xmdpy->mutex);
   stat = get_drawable_size(dpy, b->ws.drawable, width, height);
   pipe_mutex_unlock(xmdpy->mutex);

   if (!stat) {
      /* probably querying a window that's recently been destroyed */
      _mesa_warning(NULL, "XGetGeometry failed!\n");
      *width = *height = 1;
   }
}

#define GET_REDMASK(__v)        __v->mesa_visual.redMask
#define GET_GREENMASK(__v)      __v->mesa_visual.greenMask
#define GET_BLUEMASK(__v)       __v->mesa_visual.blueMask


/**
 * Choose the pixel format for the given visual.
 * This will tell the gallium driver how to pack pixel data into
 * drawing surfaces.
 */
static GLuint
choose_pixel_format(XMesaVisual v)
{
   boolean native_byte_order = (host_byte_order() == 
                                ImageByteOrder(v->display));

   if (   GET_REDMASK(v)   == 0x0000ff
       && GET_GREENMASK(v) == 0x00ff00
       && GET_BLUEMASK(v)  == 0xff0000
       && v->BitsPerPixel == 32) {
      if (native_byte_order) {
         /* no byteswapping needed */
         return PIPE_FORMAT_R8G8B8A8_UNORM;
      }
      else {
         return PIPE_FORMAT_A8B8G8R8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0xff0000
            && GET_GREENMASK(v) == 0x00ff00
            && GET_BLUEMASK(v)  == 0x0000ff
            && v->BitsPerPixel == 32) {
      if (native_byte_order) {
         /* no byteswapping needed */
         return PIPE_FORMAT_B8G8R8A8_UNORM;
      }
      else {
         return PIPE_FORMAT_A8R8G8B8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0x0000ff00
            && GET_GREENMASK(v) == 0x00ff0000
            && GET_BLUEMASK(v)  == 0xff000000
            && v->BitsPerPixel == 32) {
      if (native_byte_order) {
         /* no byteswapping needed */
         return PIPE_FORMAT_A8R8G8B8_UNORM;
      }
      else {
         return PIPE_FORMAT_B8G8R8A8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0xf800
            && GET_GREENMASK(v) == 0x07e0
            && GET_BLUEMASK(v)  == 0x001f
            && native_byte_order
            && v->BitsPerPixel == 16) {
      /* 5-6-5 RGB */
      return PIPE_FORMAT_B5G6R5_UNORM;
   }

   return PIPE_FORMAT_NONE;
}


/**
 * Choose a depth/stencil format that satisfies the given depth and
 * stencil sizes.
 */
static enum pipe_format
choose_depth_stencil_format(XMesaDisplay xmdpy, int depth, int stencil)
{
   const enum pipe_texture_target target = PIPE_TEXTURE_2D;
   const unsigned tex_usage = PIPE_BIND_DEPTH_STENCIL;
   const unsigned sample_count = 0;
   enum pipe_format formats[8], fmt;
   int count, i;

   count = 0;

   if (depth <= 16 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_Z16_UNORM;
   }
   if (depth <= 24 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_X8Z24_UNORM;
      formats[count++] = PIPE_FORMAT_Z24X8_UNORM;
   }
   if (depth <= 24 && stencil <= 8) {
      formats[count++] = PIPE_FORMAT_S8_UINT_Z24_UNORM;
      formats[count++] = PIPE_FORMAT_Z24_UNORM_S8_UINT;
   }
   if (depth <= 32 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_Z32_UNORM;
   }

   fmt = PIPE_FORMAT_NONE;
   for (i = 0; i < count; i++) {
      if (xmdpy->screen->is_format_supported(xmdpy->screen, formats[i],
                                             target, sample_count,
                                             tex_usage)) {
         fmt = formats[i];
         break;
      }
   }

   return fmt;
}



/**********************************************************************/
/*****                Linked list of XMesaBuffers                 *****/
/**********************************************************************/

static XMesaBuffer XMesaBufferList = NULL;


/**
 * Allocate a new XMesaBuffer object which corresponds to the given drawable.
 * Note that XMesaBuffer is derived from struct gl_framebuffer.
 * The new XMesaBuffer will not have any size (Width=Height=0).
 *
 * \param d  the corresponding X drawable (window or pixmap)
 * \param type  either WINDOW, PIXMAP or PBUFFER, describing d
 * \param vis  the buffer's visual
 * \param cmap  the window's colormap, if known.
 * \return new XMesaBuffer or NULL if any problem
 */
static XMesaBuffer
create_xmesa_buffer(Drawable d, BufferType type,
                    XMesaVisual vis, Colormap cmap)
{
   XMesaDisplay xmdpy = xmesa_init_display(vis->display);
   XMesaBuffer b;
   uint width, height;

   ASSERT(type == WINDOW || type == PIXMAP || type == PBUFFER);

   if (!xmdpy)
      return NULL;

   b = (XMesaBuffer) CALLOC_STRUCT(xmesa_buffer);
   if (!b)
      return NULL;

   b->ws.drawable = d;
   b->ws.visual = vis->visinfo->visual;
   b->ws.depth = vis->visinfo->depth;

   b->xm_visual = vis;
   b->type = type;
   b->cmap = cmap;

   get_drawable_size(vis->display, d, &width, &height);

   /*
    * Create framebuffer, but we'll plug in our own renderbuffers below.
    */
   b->stfb = xmesa_create_st_framebuffer(xmdpy, b);

   /* GLX_EXT_texture_from_pixmap */
   b->TextureTarget = 0;
   b->TextureFormat = GLX_TEXTURE_FORMAT_NONE_EXT;
   b->TextureMipmap = 0;

   /* insert buffer into linked list */
   b->Next = XMesaBufferList;
   XMesaBufferList = b;

   return b;
}


/**
 * Find an XMesaBuffer by matching X display and colormap but NOT matching
 * the notThis buffer.
 */
XMesaBuffer
xmesa_find_buffer(Display *dpy, Colormap cmap, XMesaBuffer notThis)
{
   XMesaBuffer b;
   for (b = XMesaBufferList; b; b = b->Next) {
      if (b->xm_visual->display == dpy &&
          b->cmap == cmap &&
          b != notThis) {
         return b;
      }
   }
   return NULL;
}


/**
 * Remove buffer from linked list, delete if no longer referenced.
 */
static void
xmesa_free_buffer(XMesaBuffer buffer)
{
   XMesaBuffer prev = NULL, b;

   for (b = XMesaBufferList; b; b = b->Next) {
      if (b == buffer) {
         /* unlink buffer from list */
         if (prev)
            prev->Next = buffer->Next;
         else
            XMesaBufferList = buffer->Next;

         /* Since the X window for the XMesaBuffer is going away, we don't
          * want to dereference this pointer in the future.
          */
         b->ws.drawable = 0;

         /* XXX we should move the buffer to a delete-pending list and destroy
          * the buffer until it is no longer current.
          */
         xmesa_destroy_st_framebuffer(buffer->stfb);

         free(buffer);

         return;
      }
      /* continue search */
      prev = b;
   }
   /* buffer not found in XMesaBufferList */
   _mesa_problem(NULL,"xmesa_free_buffer() - buffer not found\n");
}



/**********************************************************************/
/*****                   Misc Private Functions                   *****/
/**********************************************************************/


/**
 * When a context is bound for the first time, we can finally finish
 * initializing the context's visual and buffer information.
 * \param v  the XMesaVisual to initialize
 * \param b  the XMesaBuffer to initialize (may be NULL)
 * \param rgb_flag  TRUE = RGBA mode, FALSE = color index mode
 * \param window  the window/pixmap we're rendering into
 * \param cmap  the colormap associated with the window/pixmap
 * \return GL_TRUE=success, GL_FALSE=failure
 */
static GLboolean
initialize_visual_and_buffer(XMesaVisual v, XMesaBuffer b,
                             GLboolean rgb_flag, Drawable window,
                             Colormap cmap)
{
   ASSERT(!b || b->xm_visual == v);

   /* Save true bits/pixel */
   v->BitsPerPixel = bits_per_pixel(v);
   assert(v->BitsPerPixel > 0);

   if (rgb_flag == GL_FALSE) {
      /* COLOR-INDEXED WINDOW: not supported*/
      return GL_FALSE;
   }
   else {
      /* RGB WINDOW:
       * We support RGB rendering into almost any kind of visual.
       */
      const int xclass = v->visualType;
      if (xclass != GLX_TRUE_COLOR && xclass == !GLX_DIRECT_COLOR) {
	 _mesa_warning(NULL,
            "XMesa: RGB mode rendering not supported in given visual.\n");
	 return GL_FALSE;
      }
      v->mesa_visual.indexBits = 0;

      if (v->BitsPerPixel == 32) {
         /* We use XImages for all front/back buffers.  If an X Window or
          * X Pixmap is 32bpp, there's no guarantee that the alpha channel
          * will be preserved.  For XImages we're in luck.
          */
         v->mesa_visual.alphaBits = 8;
      }
   }

   /*
    * If MESA_INFO env var is set print out some debugging info
    * which can help Brian figure out what's going on when a user
    * reports bugs.
    */
   if (_mesa_getenv("MESA_INFO")) {
      printf("X/Mesa visual = %p\n", (void *) v);
      printf("X/Mesa level = %d\n", v->mesa_visual.level);
      printf("X/Mesa depth = %d\n", v->visinfo->depth);
      printf("X/Mesa bits per pixel = %d\n", v->BitsPerPixel);
   }

   return GL_TRUE;
}



#define NUM_VISUAL_TYPES   6

/**
 * Convert an X visual type to a GLX visual type.
 * 
 * \param visualType X visual type (i.e., \c TrueColor, \c StaticGray, etc.)
 *        to be converted.
 * \return If \c visualType is a valid X visual type, a GLX visual type will
 *         be returned.  Otherwise \c GLX_NONE will be returned.
 * 
 * \note
 * This code was lifted directly from lib/GL/glx/glcontextmodes.c in the
 * DRI CVS tree.
 */
static GLint
xmesa_convert_from_x_visual_type( int visualType )
{
    static const int glx_visual_types[ NUM_VISUAL_TYPES ] = {
	GLX_STATIC_GRAY,  GLX_GRAY_SCALE,
	GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
	GLX_TRUE_COLOR,   GLX_DIRECT_COLOR
    };

    return ( (unsigned) visualType < NUM_VISUAL_TYPES )
	? glx_visual_types[ visualType ] : GLX_NONE;
}


/**********************************************************************/
/*****                       Public Functions                     *****/
/**********************************************************************/


/*
 * Create a new X/Mesa visual.
 * Input:  display - X11 display
 *         visinfo - an XVisualInfo pointer
 *         rgb_flag - GL_TRUE = RGB mode,
 *                    GL_FALSE = color index mode
 *         alpha_flag - alpha buffer requested?
 *         db_flag - GL_TRUE = double-buffered,
 *                   GL_FALSE = single buffered
 *         stereo_flag - stereo visual?
 *         ximage_flag - GL_TRUE = use an XImage for back buffer,
 *                       GL_FALSE = use an off-screen pixmap for back buffer
 *         depth_size - requested bits/depth values, or zero
 *         stencil_size - requested bits/stencil values, or zero
 *         accum_red_size - requested bits/red accum values, or zero
 *         accum_green_size - requested bits/green accum values, or zero
 *         accum_blue_size - requested bits/blue accum values, or zero
 *         accum_alpha_size - requested bits/alpha accum values, or zero
 *         num_samples - number of samples/pixel if multisampling, or zero
 *         level - visual level, usually 0
 *         visualCaveat - ala the GLX extension, usually GLX_NONE
 * Return;  a new XMesaVisual or 0 if error.
 */
PUBLIC
XMesaVisual XMesaCreateVisual( Display *display,
                               XVisualInfo * visinfo,
                               GLboolean rgb_flag,
                               GLboolean alpha_flag,
                               GLboolean db_flag,
                               GLboolean stereo_flag,
                               GLboolean ximage_flag,
                               GLint depth_size,
                               GLint stencil_size,
                               GLint accum_red_size,
                               GLint accum_green_size,
                               GLint accum_blue_size,
                               GLint accum_alpha_size,
                               GLint num_samples,
                               GLint level,
                               GLint visualCaveat )
{
   XMesaDisplay xmdpy = xmesa_init_display(display);
   XMesaVisual v;
   GLint red_bits, green_bits, blue_bits, alpha_bits;

   if (!xmdpy)
      return NULL;

   /* For debugging only */
   if (_mesa_getenv("MESA_XSYNC")) {
      /* This makes debugging X easier.
       * In your debugger, set a breakpoint on _XError to stop when an
       * X protocol error is generated.
       */
      XSynchronize( display, 1 );
   }

   v = (XMesaVisual) CALLOC_STRUCT(xmesa_visual);
   if (!v) {
      return NULL;
   }

   v->display = display;

   /* Save a copy of the XVisualInfo struct because the user may Xfree()
    * the struct but we may need some of the information contained in it
    * at a later time.
    */
   v->visinfo = (XVisualInfo *) MALLOC(sizeof(*visinfo));
   if (!v->visinfo) {
      free(v);
      return NULL;
   }
   memcpy(v->visinfo, visinfo, sizeof(*visinfo));

   v->ximage_flag = ximage_flag;

   v->mesa_visual.redMask = visinfo->red_mask;
   v->mesa_visual.greenMask = visinfo->green_mask;
   v->mesa_visual.blueMask = visinfo->blue_mask;
   v->visualID = visinfo->visualid;
   v->screen = visinfo->screen;

#if !(defined(__cplusplus) || defined(c_plusplus))
   v->visualType = xmesa_convert_from_x_visual_type(visinfo->class);
#else
   v->visualType = xmesa_convert_from_x_visual_type(visinfo->c_class);
#endif

   v->mesa_visual.visualRating = visualCaveat;

   if (alpha_flag)
      v->mesa_visual.alphaBits = 8;

   (void) initialize_visual_and_buffer( v, NULL, rgb_flag, 0, 0 );

   {
      const int xclass = v->visualType;
      if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
         red_bits   = _mesa_bitcount(GET_REDMASK(v));
         green_bits = _mesa_bitcount(GET_GREENMASK(v));
         blue_bits  = _mesa_bitcount(GET_BLUEMASK(v));
      }
      else {
         /* this is an approximation */
         int depth;
         depth = v->visinfo->depth;
         red_bits = depth / 3;
         depth -= red_bits;
         green_bits = depth / 2;
         depth -= green_bits;
         blue_bits = depth;
         alpha_bits = 0;
         assert( red_bits + green_bits + blue_bits == v->visinfo->depth );
      }
      alpha_bits = v->mesa_visual.alphaBits;
   }

   /* initialize visual */
   {
      struct gl_config *vis = &v->mesa_visual;

      vis->rgbMode          = GL_TRUE;
      vis->doubleBufferMode = db_flag;
      vis->stereoMode       = stereo_flag;

      vis->redBits          = red_bits;
      vis->greenBits        = green_bits;
      vis->blueBits         = blue_bits;
      vis->alphaBits        = alpha_bits;
      vis->rgbBits          = red_bits + green_bits + blue_bits;

      vis->indexBits      = 0;
      vis->depthBits      = depth_size;
      vis->stencilBits    = stencil_size;

      vis->accumRedBits   = accum_red_size;
      vis->accumGreenBits = accum_green_size;
      vis->accumBlueBits  = accum_blue_size;
      vis->accumAlphaBits = accum_alpha_size;

      vis->haveAccumBuffer   = accum_red_size > 0;
      vis->haveDepthBuffer   = depth_size > 0;
      vis->haveStencilBuffer = stencil_size > 0;

      vis->numAuxBuffers = 0;
      vis->level = 0;
      vis->sampleBuffers = 0;
      vis->samples = 0;
   }

   v->stvis.buffer_mask = ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (db_flag)
      v->stvis.buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;
   if (stereo_flag) {
      v->stvis.buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
      if (db_flag)
         v->stvis.buffer_mask |= ST_ATTACHMENT_BACK_RIGHT_MASK;
   }

   v->stvis.color_format = choose_pixel_format(v);
   if (v->stvis.color_format == PIPE_FORMAT_NONE) {
      FREE(v->visinfo);
      FREE(v);
      return NULL;
   }

   v->stvis.depth_stencil_format =
      choose_depth_stencil_format(xmdpy, depth_size, stencil_size);

   v->stvis.accum_format = (accum_red_size +
         accum_green_size + accum_blue_size + accum_alpha_size) ?
      PIPE_FORMAT_R16G16B16A16_SNORM : PIPE_FORMAT_NONE;

   v->stvis.samples = num_samples;
   v->stvis.render_buffer = ST_ATTACHMENT_INVALID;

   /* XXX minor hack */
   v->mesa_visual.level = level;
   return v;
}


PUBLIC
void XMesaDestroyVisual( XMesaVisual v )
{
   free(v->visinfo);
   free(v);
}


/**
 * Return the informative name.
 */
const char *
xmesa_get_name(void)
{
   return stapi->name;
}


/**
 * Do per-display initializations.
 */
void
xmesa_init( Display *display )
{
   xmesa_init_display(display);
}


/**
 * Create a new XMesaContext.
 * \param v  the XMesaVisual
 * \param share_list  another XMesaContext with which to share display
 *                    lists or NULL if no sharing is wanted.
 * \return an XMesaContext or NULL if error.
 */
PUBLIC
XMesaContext XMesaCreateContext( XMesaVisual v, XMesaContext share_list,
                                 GLuint major, GLuint minor,
                                 GLuint profileMask, GLuint contextFlags)
{
   XMesaDisplay xmdpy = xmesa_init_display(v->display);
   struct st_context_attribs attribs;
   enum st_context_error ctx_err = 0;
   XMesaContext c;

   if (!xmdpy)
      return NULL;

   /* Note: the XMesaContext contains a Mesa struct gl_context struct (inheritance) */
   c = (XMesaContext) CALLOC_STRUCT(xmesa_context);
   if (!c)
      return NULL;

   c->xm_visual = v;
   c->xm_buffer = NULL;   /* set later by XMesaMakeCurrent */
   c->xm_read_buffer = NULL;

   memset(&attribs, 0, sizeof(attribs));
   attribs.visual = v->stvis;
   attribs.major = major;
   attribs.minor = minor;
   if (contextFlags & GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
      attribs.flags |= ST_CONTEXT_FLAG_FORWARD_COMPATIBLE;
   if (contextFlags & GLX_CONTEXT_DEBUG_BIT_ARB)
      attribs.flags |= ST_CONTEXT_FLAG_DEBUG;
   if (contextFlags & GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB)
      attribs.flags |= ST_CONTEXT_FLAG_ROBUST_ACCESS;

   /* There are no profiles before OpenGL 3.2.  The
    * GLX_ARB_create_context_profile spec says:
    *
    *     "If the requested OpenGL version is less than 3.2,
    *     GLX_CONTEXT_PROFILE_MASK_ARB is ignored and the functionality of the
    *     context is determined solely by the requested version."
    *
    * The spec also says:
    *
    *     "The default value for GLX_CONTEXT_PROFILE_MASK_ARB is
    *     GLX_CONTEXT_CORE_PROFILE_BIT_ARB."
    */
   attribs.profile = ST_PROFILE_DEFAULT;
   if ((major > 3 || (major == 3 && minor >= 2))
       && ((profileMask & GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB) == 0))
      attribs.profile = ST_PROFILE_OPENGL_CORE;

   c->st = stapi->create_context(stapi, xmdpy->smapi, &attribs,
         &ctx_err, (share_list) ? share_list->st : NULL);
   if (c->st == NULL)
      goto fail;

   c->st->st_manager_private = (void *) c;

   return c;

fail:
   if (c->st)
      c->st->destroy(c->st);

   free(c);
   return NULL;
}



PUBLIC
void XMesaDestroyContext( XMesaContext c )
{
   c->st->destroy(c->st);

   /* FIXME: We should destroy the screen here, but if we do so, surfaces may 
    * outlive it, causing segfaults
   struct pipe_screen *screen = c->st->pipe->screen; 
   screen->destroy(screen);
   */

   free(c);
}



/**
 * Private function for creating an XMesaBuffer which corresponds to an
 * X window or pixmap.
 * \param v  the window's XMesaVisual
 * \param w  the window we're wrapping
 * \return  new XMesaBuffer or NULL if error
 */
PUBLIC XMesaBuffer
XMesaCreateWindowBuffer(XMesaVisual v, Window w)
{
   XWindowAttributes attr;
   XMesaBuffer b;
   Colormap cmap;
   int depth;

   assert(v);
   assert(w);

   /* Check that window depth matches visual depth */
   XGetWindowAttributes( v->display, w, &attr );
   depth = attr.depth;
   if (v->visinfo->depth != depth) {
      _mesa_warning(NULL, "XMesaCreateWindowBuffer: depth mismatch between visual (%d) and window (%d)!\n",
                    v->visinfo->depth, depth);
      return NULL;
   }

   /* Find colormap */
   if (attr.colormap) {
      cmap = attr.colormap;
   }
   else {
      _mesa_warning(NULL, "Window %u has no colormap!\n", (unsigned int) w);
      /* this is weird, a window w/out a colormap!? */
      /* OK, let's just allocate a new one and hope for the best */
      cmap = XCreateColormap(v->display, w, attr.visual, AllocNone);
   }

   b = create_xmesa_buffer((Drawable) w, WINDOW, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer( v, b, v->mesa_visual.rgbMode,
                                      (Drawable) w, cmap )) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



/**
 * Create a new XMesaBuffer from an X pixmap.
 *
 * \param v    the XMesaVisual
 * \param p    the pixmap
 * \param cmap the colormap, may be 0 if using a \c GLX_TRUE_COLOR or
 *             \c GLX_DIRECT_COLOR visual for the pixmap
 * \returns new XMesaBuffer or NULL if error
 */
PUBLIC XMesaBuffer
XMesaCreatePixmapBuffer(XMesaVisual v, Pixmap p, Colormap cmap)
{
   XMesaBuffer b;

   assert(v);

   b = create_xmesa_buffer((Drawable) p, PIXMAP, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer(v, b, v->mesa_visual.rgbMode,
				     (Drawable) p, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}


/**
 * For GLX_EXT_texture_from_pixmap
 */
XMesaBuffer
XMesaCreatePixmapTextureBuffer(XMesaVisual v, Pixmap p,
                               Colormap cmap,
                               int format, int target, int mipmap)
{
   GET_CURRENT_CONTEXT(ctx);
   XMesaBuffer b;

   assert(v);

   b = create_xmesa_buffer((Drawable) p, PIXMAP, v, cmap);
   if (!b)
      return NULL;

   /* get pixmap size */
   xmesa_get_window_size(v->display, b, &b->width, &b->height);

   if (target == 0) {
      /* examine dims */
      if (ctx->Extensions.ARB_texture_non_power_of_two) {
         target = GLX_TEXTURE_2D_EXT;
      }
      else if (   _mesa_bitcount(b->width)  == 1
               && _mesa_bitcount(b->height) == 1) {
         /* power of two size */
         if (b->height == 1) {
            target = GLX_TEXTURE_1D_EXT;
         }
         else {
            target = GLX_TEXTURE_2D_EXT;
         }
      }
      else if (ctx->Extensions.NV_texture_rectangle) {
         target = GLX_TEXTURE_RECTANGLE_EXT;
      }
      else {
         /* non power of two textures not supported */
         XMesaDestroyBuffer(b);
         return 0;
      }
   }

   b->TextureTarget = target;
   b->TextureFormat = format;
   b->TextureMipmap = mipmap;

   if (!initialize_visual_and_buffer(v, b, v->mesa_visual.rgbMode,
				     (Drawable) p, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



XMesaBuffer
XMesaCreatePBuffer(XMesaVisual v, Colormap cmap,
                   unsigned int width, unsigned int height)
{
   Window root;
   Drawable drawable;  /* X Pixmap Drawable */
   XMesaBuffer b;

   /* allocate pixmap for front buffer */
   root = RootWindow( v->display, v->visinfo->screen );
   drawable = XCreatePixmap(v->display, root, width, height,
                            v->visinfo->depth);
   if (!drawable)
      return NULL;

   b = create_xmesa_buffer(drawable, PBUFFER, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer(v, b, v->mesa_visual.rgbMode,
				     drawable, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



/*
 * Deallocate an XMesaBuffer structure and all related info.
 */
PUBLIC void
XMesaDestroyBuffer(XMesaBuffer b)
{
   xmesa_free_buffer(b);
}


/**
 * Notify the binding context to validate the buffer.
 */
void
xmesa_notify_invalid_buffer(XMesaBuffer b)
{
   p_atomic_inc(&b->stfb->stamp);
}


/**
 * Query the current drawable size and notify the binding context.
 */
void
xmesa_check_buffer_size(XMesaBuffer b)
{
   GLuint old_width, old_height;

   if (b->type == PBUFFER)
      return;

   old_width = b->width;
   old_height = b->height;

   xmesa_get_window_size(b->xm_visual->display, b, &b->width, &b->height);

   if (b->width != old_width || b->height != old_height)
      xmesa_notify_invalid_buffer(b);
}


/*
 * Bind buffer b to context c and make c the current rendering context.
 */
PUBLIC
GLboolean XMesaMakeCurrent2( XMesaContext c, XMesaBuffer drawBuffer,
                             XMesaBuffer readBuffer )
{
   XMesaContext old_ctx = XMesaGetCurrentContext();

   if (old_ctx && old_ctx != c) {
      XMesaFlush(old_ctx);
      old_ctx->xm_buffer = NULL;
      old_ctx->xm_read_buffer = NULL;
   }

   if (c) {
      if (!drawBuffer || !readBuffer)
         return GL_FALSE;  /* must specify buffers! */

      if (c == old_ctx &&
	  c->xm_buffer == drawBuffer &&
	  c->xm_read_buffer == readBuffer)
	 return GL_TRUE;

      xmesa_check_buffer_size(drawBuffer);
      if (readBuffer != drawBuffer)
         xmesa_check_buffer_size(readBuffer);

      c->xm_buffer = drawBuffer;
      c->xm_read_buffer = readBuffer;

      stapi->make_current(stapi, c->st, drawBuffer->stfb, readBuffer->stfb);

      /* Solution to Stephane Rehel's problem with glXReleaseBuffersMESA(): */
      drawBuffer->wasCurrent = GL_TRUE;
   }
   else {
      /* Detach */
      stapi->make_current(stapi, NULL, NULL, NULL);

   }
   return GL_TRUE;
}


/*
 * Unbind the context c from its buffer.
 */
GLboolean XMesaUnbindContext( XMesaContext c )
{
   /* A no-op for XFree86 integration purposes */
   return GL_TRUE;
}


XMesaContext XMesaGetCurrentContext( void )
{
   struct st_context_iface *st = stapi->get_current(stapi);
   return (XMesaContext) (st) ? st->st_manager_private : NULL;
}



/**
 * Swap front and back color buffers and have winsys display front buffer.
 * If there's no front color buffer no swap actually occurs.
 */
PUBLIC
void XMesaSwapBuffers( XMesaBuffer b )
{
   XMesaContext xmctx = XMesaGetCurrentContext();

   if (xmctx && xmctx->xm_buffer == b) {
      xmctx->st->flush( xmctx->st, ST_FLUSH_FRONT, NULL);
   }

   xmesa_swap_st_framebuffer(b->stfb);
}



/*
 * Copy sub-region of back buffer to front buffer
 */
void XMesaCopySubBuffer( XMesaBuffer b, int x, int y, int width, int height )
{
   xmesa_copy_st_framebuffer(b->stfb,
         ST_ATTACHMENT_BACK_LEFT, ST_ATTACHMENT_FRONT_LEFT,
         x, y, width, height);
}



void XMesaFlush( XMesaContext c )
{
   if (c && c->xm_visual->display) {
      XMesaDisplay xmdpy = xmesa_init_display(c->xm_visual->display);
      struct pipe_fence_handle *fence = NULL;

      c->st->flush(c->st, ST_FLUSH_FRONT, &fence);
      if (fence) {
         xmdpy->screen->fence_finish(xmdpy->screen, fence,
                                     PIPE_TIMEOUT_INFINITE);
         xmdpy->screen->fence_reference(xmdpy->screen, &fence, NULL);
      }
      XFlush( c->xm_visual->display );
   }
}





XMesaBuffer XMesaFindBuffer( Display *dpy, Drawable d )
{
   XMesaBuffer b;
   for (b = XMesaBufferList; b; b = b->Next) {
      if (b->ws.drawable == d && b->xm_visual->display == dpy) {
         return b;
      }
   }
   return NULL;
}


/**
 * Free/destroy all XMesaBuffers associated with given display.
 */
void xmesa_destroy_buffers_on_display(Display *dpy)
{
   XMesaBuffer b, next;
   for (b = XMesaBufferList; b; b = next) {
      next = b->Next;
      if (b->xm_visual->display == dpy) {
         xmesa_free_buffer(b);
         /* delete head of list? */
         if (XMesaBufferList == b) {
            XMesaBufferList = next;
         }
      }
   }
}


/*
 * Look for XMesaBuffers whose X window has been destroyed.
 * Deallocate any such XMesaBuffers.
 */
void XMesaGarbageCollect( void )
{
   XMesaBuffer b, next;
   for (b=XMesaBufferList; b; b=next) {
      next = b->Next;
      if (b->xm_visual &&
          b->xm_visual->display &&
          b->ws.drawable &&
          b->type == WINDOW) {
         XSync(b->xm_visual->display, False);
         if (!window_exists( b->xm_visual->display, b->ws.drawable )) {
            /* found a dead window, free the ancillary info */
            XMesaDestroyBuffer( b );
         }
      }
   }
}


static enum st_attachment_type xmesa_attachment_type(int glx_attachment)
{
   switch(glx_attachment) {
      case GLX_FRONT_LEFT_EXT:
         return ST_ATTACHMENT_FRONT_LEFT;
      case GLX_FRONT_RIGHT_EXT:
         return ST_ATTACHMENT_FRONT_RIGHT;
      case GLX_BACK_LEFT_EXT:
         return ST_ATTACHMENT_BACK_LEFT;
      case GLX_BACK_RIGHT_EXT:
         return ST_ATTACHMENT_BACK_RIGHT;
      default:
         assert(0);
         return ST_ATTACHMENT_FRONT_LEFT;
   }
}


PUBLIC void
XMesaBindTexImage(Display *dpy, XMesaBuffer drawable, int buffer,
                  const int *attrib_list)
{
   struct st_context_iface *st = stapi->get_current(stapi);
   struct st_framebuffer_iface* stfbi = drawable->stfb;
   struct pipe_resource *res;
   int x, y, w, h;
   enum st_attachment_type st_attachment = xmesa_attachment_type(buffer);

   x = 0;
   y = 0;
   w = drawable->width;
   h = drawable->height;

   /* We need to validate our attachments before using them,
    * in case the texture doesn't exist yet. */
   xmesa_st_framebuffer_validate_textures(stfbi, w, h, 1 << st_attachment);
   res = xmesa_get_attachment(stfbi, st_attachment);

   if (res) {
      struct pipe_context* pipe = xmesa_get_context(stfbi);
      enum pipe_format internal_format = res->format;
      struct pipe_transfer *tex_xfer;
      char *map;
      int line, ximage_stride;
      XImage *img;

      internal_format = choose_pixel_format(drawable->xm_visual);

      tex_xfer = pipe_get_transfer(pipe, res,
                                   0, 0,    /* level, layer */
                                   PIPE_TRANSFER_WRITE,
                                   x, y,
                                   w, h);
      if (!tex_xfer)
         return;

      /* Grab the XImage that we want to turn into a texture. */
      img = XGetImage(dpy,
                      drawable->ws.drawable,
                      x, y,
                      w, h,
                      AllPlanes,
                      ZPixmap);

      if (!img) {
         pipe_transfer_destroy(pipe, tex_xfer);
         return;
      }

      map = pipe_transfer_map(pipe, tex_xfer);

      if (!map) {
         pipe_transfer_destroy(pipe, tex_xfer);
         return;
      }

      /* The pipe transfer has a pitch rounded up to the nearest 64 pixels.
         We assume 32 bit pixels. */
      ximage_stride = w * 4;

      for (line = 0; line < h; line++)
         memcpy(&map[line * tex_xfer->stride],
                &img->data[line * ximage_stride],
                ximage_stride);

      pipe_transfer_unmap(pipe, tex_xfer);

      pipe_transfer_destroy(pipe, tex_xfer);

      st->teximage(st,
                   ST_TEXTURE_2D,
                   0,    /* level */
                   internal_format,
                   res,
                   FALSE /* no mipmap */);

   }
}



PUBLIC void
XMesaReleaseTexImage(Display *dpy, XMesaBuffer drawable, int buffer)
{
}


void
XMesaCopyContext(XMesaContext src, XMesaContext dst, unsigned long mask)
{
   if (dst->st->copy)
      dst->st->copy(dst->st, src->st, mask);
}
