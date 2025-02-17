/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/light.h"
#include "main/state.h"

#include "vbo_context.h"


#if FEATURE_dlist


/**
 * After playback, copy everything but the position from the
 * last vertex to the saved state
 */
static void
_playback_copy_to_current(struct gl_context *ctx,
                          const struct vbo_save_vertex_list *node)
{
   struct vbo_context *vbo = vbo_context(ctx);
   GLfloat vertex[VBO_ATTRIB_MAX * 4];
   GLfloat *data;
   GLuint i, offset;

   if (node->current_size == 0)
      return;

   if (node->current_data) {
      data = node->current_data;
   }
   else {
      data = vertex;

      if (node->count)
         offset = (node->buffer_offset + 
                   (node->count-1) * node->vertex_size * sizeof(GLfloat));
      else
         offset = node->buffer_offset;

      ctx->Driver.GetBufferSubData( ctx, offset,
                                    node->vertex_size * sizeof(GLfloat), 
                                    data, node->vertex_store->bufferobj );

      data += node->attrsz[0]; /* skip vertex position */
   }

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      if (node->attrsz[i]) {
	 GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;
         GLfloat tmp[4];

         COPY_CLEAN_4V_TYPE_AS_FLOAT(tmp,
                                     node->attrsz[i],
                                     data,
                                     node->attrtype[i]);
         
         if (node->attrtype[i] != vbo->currval[i].Type ||
             memcmp(current, tmp, 4 * sizeof(GLfloat)) != 0) {
            memcpy(current, tmp, 4 * sizeof(GLfloat));

            vbo->currval[i].Size = node->attrsz[i];
            vbo->currval[i]._ElementSize = vbo->currval[i].Size * sizeof(GLfloat);
            vbo->currval[i].Type = node->attrtype[i];
            vbo->currval[i].Integer =
                  vbo_attrtype_to_integer_flag(node->attrtype[i]);

            if (i >= VBO_ATTRIB_FIRST_MATERIAL &&
                i <= VBO_ATTRIB_LAST_MATERIAL)
               ctx->NewState |= _NEW_LIGHT;

            ctx->NewState |= _NEW_CURRENT_ATTRIB;
         }

	 data += node->attrsz[i];
      }
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx, ctx->Current.Attrib[VBO_ATTRIB_COLOR0]);
   }

   /* CurrentExecPrimitive
    */
   if (node->prim_count) {
      const struct _mesa_prim *prim = &node->prim[node->prim_count - 1];
      if (prim->end)
	 ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
      else
	 ctx->Driver.CurrentExecPrimitive = prim->mode;
   }
}



/**
 * Treat the vertex storage as a VBO, define vertex arrays pointing
 * into it:
 */
static void vbo_bind_vertex_list(struct gl_context *ctx,
                                 const struct vbo_save_vertex_list *node)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_save_context *save = &vbo->save;
   struct gl_client_array *arrays = save->arrays;
   GLuint buffer_offset = node->buffer_offset;
   const GLuint *map;
   GLuint attr;
   GLubyte node_attrsz[VBO_ATTRIB_MAX];  /* copy of node->attrsz[] */
   GLenum node_attrtype[VBO_ATTRIB_MAX];  /* copy of node->attrtype[] */
   GLbitfield64 varying_inputs = 0x0;

   memcpy(node_attrsz, node->attrsz, sizeof(node->attrsz));
   memcpy(node_attrtype, node->attrtype, sizeof(node->attrtype));

   /* Install the default (ie Current) attributes first, then overlay
    * all active ones.
    */
   switch (get_program_mode(ctx)) {
   case VP_NONE:
      for (attr = 0; attr < VERT_ATTRIB_FF_MAX; attr++) {
         save->inputs[attr] = &vbo->currval[VBO_ATTRIB_POS+attr];
      }
      for (attr = 0; attr < MAT_ATTRIB_MAX; attr++) {
         save->inputs[VERT_ATTRIB_GENERIC(attr)] =
            &vbo->currval[VBO_ATTRIB_MAT_FRONT_AMBIENT+attr];
      }
      map = vbo->map_vp_none;
      break;
   case VP_NV:
   case VP_ARB:
      /* The aliasing of attributes for NV vertex programs has already
       * occurred.  NV vertex programs cannot access material values,
       * nor attributes greater than VERT_ATTRIB_TEX7.  
       */
      for (attr = 0; attr < VERT_ATTRIB_FF_MAX; attr++) {
         save->inputs[attr] = &vbo->currval[VBO_ATTRIB_POS+attr];
      }
      for (attr = 0; attr < VERT_ATTRIB_GENERIC_MAX; attr++) {
         save->inputs[VERT_ATTRIB_GENERIC(attr)] =
            &vbo->currval[VBO_ATTRIB_GENERIC0+attr];
      }
      map = vbo->map_vp_arb;

      /* check if VERT_ATTRIB_POS is not read but VERT_BIT_GENERIC0 is read.
       * In that case we effectively need to route the data from
       * glVertexAttrib(0, val) calls to feed into the GENERIC0 input.
       */
      if ((ctx->VertexProgram._Current->Base.InputsRead & VERT_BIT_POS) == 0 &&
          (ctx->VertexProgram._Current->Base.InputsRead & VERT_BIT_GENERIC0)) {
         save->inputs[VERT_ATTRIB_GENERIC0] = save->inputs[0];
         node_attrsz[VERT_ATTRIB_GENERIC0] = node_attrsz[0];
         node_attrtype[VERT_ATTRIB_GENERIC0] = node_attrtype[0];
         node_attrsz[0] = 0;
      }
      break;
   default:
      assert(0);
   }

   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      const GLuint src = map[attr];

      if (node_attrsz[src]) {
         /* override the default array set above */
         save->inputs[attr] = &arrays[attr];

	 arrays[attr].Ptr = (const GLubyte *) NULL + buffer_offset;
	 arrays[attr].Size = node_attrsz[src];
	 arrays[attr].StrideB = node->vertex_size * sizeof(GLfloat);
	 arrays[attr].Stride = node->vertex_size * sizeof(GLfloat);
         arrays[attr].Type = node_attrtype[src];
         arrays[attr].Integer =
               vbo_attrtype_to_integer_flag(node_attrtype[src]);
         arrays[attr].Format = GL_RGBA;
	 arrays[attr].Enabled = 1;
         arrays[attr]._ElementSize = arrays[attr].Size * sizeof(GLfloat);
         _mesa_reference_buffer_object(ctx,
                                       &arrays[attr].BufferObj,
                                       node->vertex_store->bufferobj);
	 arrays[attr]._MaxElement = node->count; /* ??? */
	 
	 assert(arrays[attr].BufferObj->Name);

	 buffer_offset += node_attrsz[src] * sizeof(GLfloat);
         varying_inputs |= VERT_BIT(attr);
      }
   }

   _mesa_set_varying_vp_inputs( ctx, varying_inputs );
   ctx->NewDriverState |= ctx->DriverFlags.NewArray;
}


static void
vbo_save_loopback_vertex_list(struct gl_context *ctx,
                              const struct vbo_save_vertex_list *list)
{
   const char *buffer =
      ctx->Driver.MapBufferRange(ctx, 0,
				 list->vertex_store->bufferobj->Size,
				 GL_MAP_READ_BIT, /* ? */
				 list->vertex_store->bufferobj);

   vbo_loopback_vertex_list(ctx,
                            (const GLfloat *)(buffer + list->buffer_offset),
                            list->attrsz,
                            list->prim,
                            list->prim_count,
                            list->wrap_count,
                            list->vertex_size);

   ctx->Driver.UnmapBuffer(ctx, list->vertex_store->bufferobj);
}


/**
 * Execute the buffer and save copied verts.
 * This is called from the display list code when executing
 * a drawing command.
 */
void
vbo_save_playback_vertex_list(struct gl_context *ctx, void *data)
{
   const struct vbo_save_vertex_list *node =
      (const struct vbo_save_vertex_list *) data;
   struct vbo_save_context *save = &vbo_context(ctx)->save;
   GLboolean remap_vertex_store = GL_FALSE;

   if (save->vertex_store->buffer) {
      /* The vertex store is currently mapped but we're about to replay
       * a display list.  This can happen when a nested display list is
       * being build with GL_COMPILE_AND_EXECUTE.
       * We never want to have mapped vertex buffers when we're drawing.
       * Unmap the vertex store, execute the list, then remap the vertex
       * store.
       */
      vbo_save_unmap_vertex_store(ctx, save->vertex_store);
      remap_vertex_store = GL_TRUE;
   }

   FLUSH_CURRENT(ctx, 0);

   if (node->prim_count > 0) {

      if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END &&
	  node->prim[0].begin) {

	 /* Degenerate case: list is called inside begin/end pair and
	  * includes operations such as glBegin or glDrawArrays.
	  */
	 if (0)
	    printf("displaylist recursive begin");

	 vbo_save_loopback_vertex_list( ctx, node );

         goto end;
      }
      else if (save->replay_flags) {
	 /* Various degnerate cases: translate into immediate mode
	  * calls rather than trying to execute in place.
	  */
	 vbo_save_loopback_vertex_list( ctx, node );

         goto end;
      }
      
      if (ctx->NewState)
	 _mesa_update_state( ctx );

      /* XXX also need to check if shader enabled, but invalid */
      if ((ctx->VertexProgram.Enabled && !ctx->VertexProgram._Enabled) ||
          (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBegin (invalid vertex/fragment program)");
         return;
      }

      vbo_bind_vertex_list( ctx, node );

      vbo_draw_method(vbo_context(ctx), DRAW_DISPLAY_LIST);

      /* Again...
       */
      if (ctx->NewState)
	 _mesa_update_state( ctx );

      if (node->count > 0) {
         vbo_context(ctx)->draw_prims(ctx, 
                                      node->prim,
                                      node->prim_count,
                                      NULL,
                                      GL_TRUE,
                                      0,    /* Node is a VBO, so this is ok */
                                      node->count - 1,
                                      NULL);
      }
   }

   /* Copy to current?
    */
   _playback_copy_to_current( ctx, node );

end:
   if (remap_vertex_store) {
      save->buffer_ptr = vbo_save_map_vertex_store(ctx, save->vertex_store);
   }
}


#endif /* FEATURE_dlist */
