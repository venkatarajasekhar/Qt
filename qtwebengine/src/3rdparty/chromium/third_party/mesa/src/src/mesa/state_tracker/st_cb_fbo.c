/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * Framebuffer/renderbuffer functions.
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/renderbuffer.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "st_context.h"
#include "st_cb_fbo.h"
#include "st_cb_flush.h"
#include "st_cb_texture.h"
#include "st_format.h"
#include "st_texture.h"
#include "st_manager.h"

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_surface.h"


static GLboolean
st_renderbuffer_alloc_sw_storage(struct gl_context * ctx,
                                 struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   enum pipe_format format;
   size_t size;

   free(strb->data);
   strb->data = NULL;

   if (internalFormat == GL_RGBA16_SNORM) {
      /* Special case for software accum buffers.  Otherwise, if the
       * call to st_choose_renderbuffer_format() fails (because the
       * driver doesn't support signed 16-bit/channel colors) we'd
       * just return without allocating the software accum buffer.
       */
      format = PIPE_FORMAT_R16G16B16A16_SNORM;
   }
   else {
      format = st_choose_renderbuffer_format(screen, internalFormat, 0);

      /* Not setting gl_renderbuffer::Format here will cause
       * FRAMEBUFFER_UNSUPPORTED and ValidateFramebuffer will not be called.
       */
      if (format == PIPE_FORMAT_NONE) {
         return GL_TRUE;
      }
   }

   strb->Base.Format = st_pipe_format_to_mesa_format(format);

   size = _mesa_format_image_size(strb->Base.Format, width, height, 1);
   strb->data = malloc(size);
   return strb->data != NULL;
}


/**
 * gl_renderbuffer::AllocStorage()
 * This is called to allocate the original drawing surface, and
 * during window resize.
 */
static GLboolean
st_renderbuffer_alloc_storage(struct gl_context * ctx,
                              struct gl_renderbuffer *rb,
                              GLenum internalFormat,
                              GLuint width, GLuint height)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = st->pipe->screen;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   enum pipe_format format = PIPE_FORMAT_NONE;
   struct pipe_surface surf_tmpl;
   struct pipe_resource templ;

   /* init renderbuffer fields */
   strb->Base.Width  = width;
   strb->Base.Height = height;
   strb->Base._BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);
   strb->defined = GL_FALSE;  /* undefined contents now */

   if (strb->software) {
      return st_renderbuffer_alloc_sw_storage(ctx, rb, internalFormat,
                                              width, height);
   }

   /* Free the old surface and texture
    */
   pipe_surface_reference( &strb->surface, NULL );
   pipe_resource_reference( &strb->texture, NULL );

   /* Handle multisample renderbuffers first.
    *
    * From ARB_framebuffer_object:
    *   If <samples> is zero, then RENDERBUFFER_SAMPLES is set to zero.
    *   Otherwise <samples> represents a request for a desired minimum
    *   number of samples. Since different implementations may support
    *   different sample counts for multisampled rendering, the actual
    *   number of samples allocated for the renderbuffer image is
    *   implementation dependent.  However, the resulting value for
    *   RENDERBUFFER_SAMPLES is guaranteed to be greater than or equal
    *   to <samples> and no more than the next larger sample count supported
    *   by the implementation.
    *
    * So let's find the supported number of samples closest to NumSamples.
    * (NumSamples == 1) is treated the same as (NumSamples == 0).
    */
   if (rb->NumSamples > 1) {
      unsigned i;

      for (i = rb->NumSamples; i <= ctx->Const.MaxSamples; i++) {
         format = st_choose_renderbuffer_format(screen, internalFormat, i);

         if (format != PIPE_FORMAT_NONE) {
            rb->NumSamples = i;
            break;
         }
      }
   } else {
      format = st_choose_renderbuffer_format(screen, internalFormat, 0);
   }

   /* Not setting gl_renderbuffer::Format here will cause
    * FRAMEBUFFER_UNSUPPORTED and ValidateFramebuffer will not be called.
    */
   if (format == PIPE_FORMAT_NONE) {
      return GL_TRUE;
   }

   strb->Base.Format = st_pipe_format_to_mesa_format(format);

   if (width == 0 || height == 0) {
      /* if size is zero, nothing to allocate */
      return GL_TRUE;
   }

   /* Setup new texture template.
    */
   memset(&templ, 0, sizeof(templ));
   templ.target = st->internal_target;
   templ.format = format;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.nr_samples = rb->NumSamples;
   if (util_format_is_depth_or_stencil(format)) {
      templ.bind = PIPE_BIND_DEPTH_STENCIL;
   }
   else if (strb->Base.Name != 0) {
      /* this is a user-created renderbuffer */
      templ.bind = PIPE_BIND_RENDER_TARGET;
   }
   else {
      /* this is a window-system buffer */
      templ.bind = (PIPE_BIND_DISPLAY_TARGET |
                    PIPE_BIND_RENDER_TARGET);
   }

   strb->texture = screen->resource_create(screen, &templ);

   if (!strb->texture)
      return FALSE;

   u_surface_default_template(&surf_tmpl, strb->texture, templ.bind);
   strb->surface = pipe->create_surface(pipe,
                                        strb->texture,
                                        &surf_tmpl);
   if (strb->surface) {
      assert(strb->surface->texture);
      assert(strb->surface->format);
      assert(strb->surface->width == width);
      assert(strb->surface->height == height);
   }

   return strb->surface != NULL;
}


/**
 * gl_renderbuffer::Delete()
 */
static void
st_renderbuffer_delete(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   if (ctx) {
      struct st_context *st = st_context(ctx);
      pipe_surface_release(st->pipe, &strb->surface);
   }
   pipe_resource_reference(&strb->texture, NULL);
   free(strb->data);
   _mesa_delete_renderbuffer(ctx, rb);
}


/**
 * Called via ctx->Driver.NewFramebuffer()
 */
static struct gl_framebuffer *
st_new_framebuffer(struct gl_context *ctx, GLuint name)
{
   /* XXX not sure we need to subclass gl_framebuffer for pipe */
   return _mesa_new_framebuffer(ctx, name);
}


/**
 * Called via ctx->Driver.NewRenderbuffer()
 */
static struct gl_renderbuffer *
st_new_renderbuffer(struct gl_context *ctx, GLuint name)
{
   struct st_renderbuffer *strb = ST_CALLOC_STRUCT(st_renderbuffer);
   if (strb) {
      assert(name != 0);
      _mesa_init_renderbuffer(&strb->Base, name);
      strb->Base.Delete = st_renderbuffer_delete;
      strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
      return &strb->Base;
   }
   return NULL;
}


/**
 * Allocate a renderbuffer for a an on-screen window (not a user-created
 * renderbuffer).  The window system code determines the format.
 */
struct gl_renderbuffer *
st_new_renderbuffer_fb(enum pipe_format format, int samples, boolean sw)
{
   struct st_renderbuffer *strb;

   strb = ST_CALLOC_STRUCT(st_renderbuffer);
   if (!strb) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&strb->Base, 0);
   strb->Base.ClassID = 0x4242; /* just a unique value */
   strb->Base.NumSamples = samples;
   strb->Base.Format = st_pipe_format_to_mesa_format(format);
   strb->Base._BaseFormat = _mesa_get_format_base_format(strb->Base.Format);
   strb->software = sw;
   
   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      strb->Base.InternalFormat = GL_RGBA8;
      break;
   case PIPE_FORMAT_R8G8B8X8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      strb->Base.InternalFormat = GL_RGB8;
      break;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      strb->Base.InternalFormat = GL_RGB5_A1;
      break;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      strb->Base.InternalFormat = GL_RGBA4;
      break;
   case PIPE_FORMAT_B5G6R5_UNORM:
      strb->Base.InternalFormat = GL_RGB565;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      strb->Base.InternalFormat = GL_DEPTH_COMPONENT16;
      break;
   case PIPE_FORMAT_Z32_UNORM:
      strb->Base.InternalFormat = GL_DEPTH_COMPONENT32;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      strb->Base.InternalFormat = GL_DEPTH24_STENCIL8_EXT;
      break;
   case PIPE_FORMAT_S8_UINT:
      strb->Base.InternalFormat = GL_STENCIL_INDEX8_EXT;
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      /* accum buffer */
      strb->Base.InternalFormat = GL_RGBA16_SNORM;
      break;
   case PIPE_FORMAT_R8_UNORM:
      strb->Base.InternalFormat = GL_R8;
      break;
   case PIPE_FORMAT_R8G8_UNORM:
      strb->Base.InternalFormat = GL_RG8;
      break;
   case PIPE_FORMAT_R16_UNORM:
      strb->Base.InternalFormat = GL_R16;
      break;
   case PIPE_FORMAT_R16G16_UNORM:
      strb->Base.InternalFormat = GL_RG16;
      break;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      strb->Base.InternalFormat = GL_RGBA32F;
      break;
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      strb->Base.InternalFormat = GL_RGBA16F;
      break;
   default:
      _mesa_problem(NULL,
		    "Unexpected format %s in st_new_renderbuffer_fb",
                    util_format_name(format));
      free(strb);
      return NULL;
   }

   /* st-specific methods */
   strb->Base.Delete = st_renderbuffer_delete;
   strb->Base.AllocStorage = st_renderbuffer_alloc_storage;

   /* surface is allocated in st_renderbuffer_alloc_storage() */
   strb->surface = NULL;

   return &strb->Base;
}


/**
 * Called via ctx->Driver.BindFramebufferEXT().
 */
static void
st_bind_framebuffer(struct gl_context *ctx, GLenum target,
                    struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
   /* no-op */
}


/**
 * Called by ctx->Driver.RenderTexture
 */
static void
st_render_texture(struct gl_context *ctx,
                  struct gl_framebuffer *fb,
                  struct gl_renderbuffer_attachment *att)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_renderbuffer *strb;
   struct gl_renderbuffer *rb;
   struct pipe_resource *pt;
   struct st_texture_object *stObj;
   const struct gl_texture_image *texImage;
   struct pipe_surface surf_tmpl;

   if (!st_finalize_texture(ctx, pipe, att->Texture))
      return;

   pt = st_get_texobj_resource(att->Texture);
   assert(pt);

   /* get pointer to texture image we're rendeing to */
   texImage = _mesa_get_attachment_teximage(att);

   /* create new renderbuffer which wraps the texture image.
    * Use the texture's name as the renderbuffer's name so that we have
    * something that's non-zero (to determine vertical orientation) and
    * possibly helpful for debugging.
    */
   rb = st_new_renderbuffer(ctx, att->Texture->Name);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture()");
      return;
   }

   _mesa_reference_renderbuffer(&att->Renderbuffer, rb);
   assert(rb->RefCount == 1);
   rb->AllocStorage = NULL; /* should not get called */
   strb = st_renderbuffer(rb);

   assert(strb->Base.RefCount > 0);

   /* get the texture for the texture object */
   stObj = st_texture_object(att->Texture);

   /* point renderbuffer at texobject */
   strb->rtt = stObj;
   strb->rtt_level = att->TextureLevel;
   strb->rtt_face = att->CubeMapFace;
   strb->rtt_slice = att->Zoffset;

   rb->Width = texImage->Width2;
   rb->Height = texImage->Height2;
   rb->_BaseFormat = texImage->_BaseFormat;
   rb->InternalFormat = texImage->InternalFormat;

   pipe_resource_reference( &strb->texture, pt );

   pipe_surface_release(pipe, &strb->surface);

   assert(strb->rtt_level <= strb->texture->last_level);

   /* new surface for rendering into the texture */
   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = ctx->Color.sRGBEnabled
      ? strb->texture->format : util_format_linear(strb->texture->format);
   surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
   surf_tmpl.u.tex.level = strb->rtt_level;
   surf_tmpl.u.tex.first_layer = strb->rtt_face + strb->rtt_slice;
   surf_tmpl.u.tex.last_layer = strb->rtt_face + strb->rtt_slice;
   strb->surface = pipe->create_surface(pipe,
                                        strb->texture,
                                        &surf_tmpl);

   strb->Base.Format = st_pipe_format_to_mesa_format(pt->format);

   /* Invalidate buffer state so that the pipe's framebuffer state
    * gets updated.
    * That's where the new renderbuffer (which we just created) gets
    * passed to the pipe as a (color/depth) render target.
    */
   st_invalidate_state(ctx, _NEW_BUFFERS);


   /* Need to trigger a call to update_framebuffer() since we just
    * attached a new renderbuffer.
    */
   ctx->NewState |= _NEW_BUFFERS;
}


/**
 * Called via ctx->Driver.FinishRenderTexture.
 */
static void
st_finish_render_texture(struct gl_context *ctx,
                         struct gl_renderbuffer_attachment *att)
{
   struct st_renderbuffer *strb = st_renderbuffer(att->Renderbuffer);

   if (!strb)
      return;

   strb->rtt = NULL;

   /* restore previous framebuffer state */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/** Debug helper */
static void
st_fbo_invalid(const char *reason)
{
   if (MESA_DEBUG_FLAGS & DEBUG_INCOMPLETE_FBO) {
      _mesa_debug(NULL, "Invalid FBO: %s\n", reason);
   }
}


/**
 * Validate a renderbuffer attachment for a particular set of bindings.
 */
static GLboolean
st_validate_attachment(struct gl_context *ctx,
		       struct pipe_screen *screen,
		       const struct gl_renderbuffer_attachment *att,
		       unsigned bindings)
{
   const struct st_texture_object *stObj = st_texture_object(att->Texture);
   enum pipe_format format;
   gl_format texFormat;
   GLboolean valid;

   /* Only validate texture attachments for now, since
    * st_renderbuffer_alloc_storage makes sure that
    * the format is supported.
    */
   if (att->Type != GL_TEXTURE)
      return GL_TRUE;

   if (!stObj)
      return GL_FALSE;

   format = stObj->pt->format;
   texFormat = _mesa_get_attachment_teximage_const(att)->TexFormat;

   /* If the encoding is sRGB and sRGB rendering cannot be enabled,
    * check for linear format support instead.
    * Later when we create a surface, we change the format to a linear one. */
   if (!ctx->Extensions.EXT_framebuffer_sRGB &&
       _mesa_get_format_color_encoding(texFormat) == GL_SRGB) {
      const gl_format linearFormat = _mesa_get_srgb_format_linear(texFormat);
      format = st_mesa_format_to_pipe_format(linearFormat);
   }

   valid = screen->is_format_supported(screen, format,
                                      PIPE_TEXTURE_2D,
                                      stObj->pt->nr_samples, bindings);
   if (!valid) {
      st_fbo_invalid("Invalid format");
   }

   return valid;
}


/**
 * Check if two renderbuffer attachments name a combined depth/stencil
 * renderbuffer.
 */
GLboolean
st_is_depth_stencil_combined(const struct gl_renderbuffer_attachment *depth,
                             const struct gl_renderbuffer_attachment *stencil)
{
   assert(depth && stencil);

   if (depth->Type == stencil->Type) {
      if (depth->Type == GL_RENDERBUFFER_EXT &&
          depth->Renderbuffer == stencil->Renderbuffer)
         return GL_TRUE;

      if (depth->Type == GL_TEXTURE &&
          depth->Texture == stencil->Texture)
         return GL_TRUE;
   }

   return GL_FALSE;
}
 

/**
 * Check that the framebuffer configuration is valid in terms of what
 * the driver can support.
 *
 * For Gallium we only supports combined Z+stencil, not separate buffers.
 */
static void
st_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->pipe->screen;
   const struct gl_renderbuffer_attachment *depth =
         &fb->Attachment[BUFFER_DEPTH];
   const struct gl_renderbuffer_attachment *stencil =
         &fb->Attachment[BUFFER_STENCIL];
   GLuint i;
   enum pipe_format first_format = PIPE_FORMAT_NONE;
   boolean mixed_formats =
         screen->get_param(screen, PIPE_CAP_MIXED_COLORBUFFER_FORMATS) != 0;

   if (depth->Type && stencil->Type && depth->Type != stencil->Type) {
      st_fbo_invalid("Different Depth/Stencil buffer formats");
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (depth->Type == GL_RENDERBUFFER_EXT &&
       stencil->Type == GL_RENDERBUFFER_EXT &&
       depth->Renderbuffer != stencil->Renderbuffer) {
      st_fbo_invalid("Separate Depth/Stencil buffers");
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (depth->Type == GL_TEXTURE &&
       stencil->Type == GL_TEXTURE &&
       depth->Texture != stencil->Texture) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      st_fbo_invalid("Different Depth/Stencil textures");
      return;
   }

   if (!st_validate_attachment(ctx,
                               screen,
                               depth,
			       PIPE_BIND_DEPTH_STENCIL)) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (!st_validate_attachment(ctx,
                               screen,
                               stencil,
			       PIPE_BIND_DEPTH_STENCIL)) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   for (i = 0; i < ctx->Const.MaxColorAttachments; i++) {
      struct gl_renderbuffer_attachment *att =
            &fb->Attachment[BUFFER_COLOR0 + i];
      enum pipe_format format;

      if (!st_validate_attachment(ctx,
                                  screen,
				  att,
				  PIPE_BIND_RENDER_TARGET)) {
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 return;
      }

      if (!mixed_formats) {
         /* Disallow mixed formats. */
         if (att->Type != GL_NONE) {
            format = st_renderbuffer(att->Renderbuffer)->surface->format;
         } else {
            continue;
         }

         if (first_format == PIPE_FORMAT_NONE) {
            first_format = format;
         } else if (format != first_format) {
            fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
            st_fbo_invalid("Mixed color formats");
            return;
         }
      }
   }
}


/**
 * Called via glDrawBuffer.
 */
static void
st_DrawBuffers(struct gl_context *ctx, GLsizei count, const GLenum *buffers)
{
   struct st_context *st = st_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint i;

   (void) count;
   (void) buffers;

   /* add the renderbuffers on demand */
   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
      gl_buffer_index idx = fb->_ColorDrawBufferIndexes[i];
      st_manager_add_color_renderbuffer(st, fb, idx);
   }
}


/**
 * Called via glReadBuffer.
 */
static void
st_ReadBuffer(struct gl_context *ctx, GLenum buffer)
{
   struct st_context *st = st_context(ctx);
   struct gl_framebuffer *fb = ctx->ReadBuffer;

   (void) buffer;

   /* add the renderbuffer on demand */
   st_manager_add_color_renderbuffer(st, fb, fb->_ColorReadBufferIndex);
}



/**
 * Called via ctx->Driver.MapRenderbuffer.
 */
static void
st_MapRenderbuffer(struct gl_context *ctx,
                   struct gl_renderbuffer *rb,
                   GLuint x, GLuint y, GLuint w, GLuint h,
                   GLbitfield mode,
                   GLubyte **mapOut, GLint *rowStrideOut)
{
   struct st_context *st = st_context(ctx);
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   struct pipe_context *pipe = st->pipe;
   const GLboolean invert = rb->Name == 0;
   unsigned usage;
   GLuint y2;

   if (strb->software) {
      /* software-allocated renderbuffer (probably an accum buffer) */
      if (strb->data) {
         GLint bpp = _mesa_get_format_bytes(strb->Base.Format);
         GLint stride = _mesa_format_row_stride(strb->Base.Format,
                                                strb->Base.Width);
         *mapOut = (GLubyte *) strb->data + y * stride + x * bpp;
         *rowStrideOut = stride;
      }
      else {
         *mapOut = NULL;
         *rowStrideOut = 0;
      }
      return;
   }

   usage = 0x0;
   if (mode & GL_MAP_READ_BIT)
      usage |= PIPE_TRANSFER_READ;
   if (mode & GL_MAP_WRITE_BIT)
      usage |= PIPE_TRANSFER_WRITE;
   if (mode & GL_MAP_INVALIDATE_RANGE_BIT)
      usage |= PIPE_TRANSFER_DISCARD_RANGE;

   /* Note: y=0=bottom of buffer while y2=0=top of buffer.
    * 'invert' will be true for window-system buffers and false for
    * user-allocated renderbuffers and textures.
    */
   if (invert)
      y2 = strb->Base.Height - y - h;
   else
      y2 = y;

   strb->transfer = pipe_get_transfer(pipe,
                                      strb->texture,
                                      strb->rtt_level,
                                      strb->rtt_face + strb->rtt_slice,
                                      usage, x, y2, w, h);
   if (strb->transfer) {
      GLubyte *map = pipe_transfer_map(pipe, strb->transfer);
      if (invert) {
         *rowStrideOut = -strb->transfer->stride;
         map += (h - 1) * strb->transfer->stride;
      }
      else {
         *rowStrideOut = strb->transfer->stride;
      }
      *mapOut = map;
   }
   else {
      *mapOut = NULL;
      *rowStrideOut = 0;
   }
}


/**
 * Called via ctx->Driver.UnmapRenderbuffer.
 */
static void
st_UnmapRenderbuffer(struct gl_context *ctx,
                     struct gl_renderbuffer *rb)
{
   struct st_context *st = st_context(ctx);
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   struct pipe_context *pipe = st->pipe;

   if (strb->software) {
      /* software-allocated renderbuffer (probably an accum buffer) */
      return;
   }

   pipe_transfer_unmap(pipe, strb->transfer);
   pipe->transfer_destroy(pipe, strb->transfer);
   strb->transfer = NULL;
}



void st_init_fbo_functions(struct dd_function_table *functions)
{
#if FEATURE_EXT_framebuffer_object
   functions->NewFramebuffer = st_new_framebuffer;
   functions->NewRenderbuffer = st_new_renderbuffer;
   functions->BindFramebuffer = st_bind_framebuffer;
   functions->FramebufferRenderbuffer = _mesa_framebuffer_renderbuffer;
   functions->RenderTexture = st_render_texture;
   functions->FinishRenderTexture = st_finish_render_texture;
   functions->ValidateFramebuffer = st_validate_framebuffer;
#endif

   functions->DrawBuffers = st_DrawBuffers;
   functions->ReadBuffer = st_ReadBuffer;

   functions->MapRenderbuffer = st_MapRenderbuffer;
   functions->UnmapRenderbuffer = st_UnmapRenderbuffer;
}


