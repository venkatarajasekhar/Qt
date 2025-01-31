/*
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Ben Skeggs
 *
 */

#include "tgsi/tgsi_parse.h"

#include "nouveau/nv_object.xml.h"
#include "nv30-40_3d.xml.h"
#include "nv30_context.h"
#include "nvfx_shader.h"

static void
nv30_fragprog_upload(struct nv30_context *nv30)
{
   struct nouveau_context *nv = &nv30->base;
   struct nv30_fragprog *fp = nv30->fragprog.program;
   struct pipe_context *pipe = &nv30->base.pipe;
   struct pipe_transfer *transfer;
   uint32_t *map;
   int i; (void)i;

   if (unlikely(!fp->buffer)) {
      fp->buffer = pipe_buffer_create(pipe->screen, 0, 0, fp->insn_len * 4);
   }

   map = pipe_buffer_map(pipe, fp->buffer, PIPE_TRANSFER_WRITE, &transfer);
#ifndef PIPE_ARCH_BIG_ENDIAN
   memcpy(map, fp->insn, fp->insn_len * 4);
#else
   for (i = 0; i < fp->insn_len; i++)
      *map++ = (fp->insn[i] >> 16) | (fp->insn[i] << 16);
#endif
   pipe_buffer_unmap(pipe, transfer);

   if (nv04_resource(fp->buffer)->domain != NOUVEAU_BO_VRAM)
      nouveau_buffer_migrate(nv, nv04_resource(fp->buffer), NOUVEAU_BO_VRAM);
}

void
nv30_fragprog_validate(struct nv30_context *nv30)
{
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
   struct nv30_fragprog *fp = nv30->fragprog.program;
   boolean upload = FALSE;
   int i;

   if (!fp->translated) {
      _nvfx_fragprog_translate(nv30, fp, FALSE);
      if (!fp->translated)
         return;

      upload = TRUE;
   }

   /* update constants, also needs to be done on every fp switch as we
    * have no idea whether the constbuf changed in the meantime
    */
   if (nv30->fragprog.constbuf) {
      struct pipe_resource *constbuf = nv30->fragprog.constbuf;
      uint32_t *cbuf = (uint32_t *)nv04_resource(constbuf)->data;

      for (i = 0; i < fp->nr_consts; i++) {
         unsigned off = fp->consts[i].offset;
         unsigned idx = fp->consts[i].index * 4;

         if (!memcmp(&fp->insn[off], &cbuf[idx], 4 * 4))
            continue;
         memcpy(&fp->insn[off], &cbuf[idx], 4 * 4);
         upload = TRUE;
      }
   }

   if (upload)
      nv30_fragprog_upload(nv30);

   /* FP_ACTIVE_PROGRAM needs to be done again even if only the consts
    * were updated.  TEX_CACHE_CTL magic is not enough to convince the
    * GPU that it should re-read the fragprog from VRAM... sigh.
    */
   if (nv30->state.fragprog != fp || upload) {
      struct nv04_resource *r = nv04_resource(fp->buffer);

      if (!PUSH_SPACE(push, 8))
         return;
      PUSH_RESET(push, BUFCTX_FRAGPROG);

      BEGIN_NV04(push, NV30_3D(FP_ACTIVE_PROGRAM), 1);
      PUSH_RESRC(push, NV30_3D(FP_ACTIVE_PROGRAM), BUFCTX_FRAGPROG, r, 0,
                       NOUVEAU_BO_LOW | NOUVEAU_BO_RD | NOUVEAU_BO_OR,
                       NV30_3D_FP_ACTIVE_PROGRAM_DMA0,
                       NV30_3D_FP_ACTIVE_PROGRAM_DMA1);
      BEGIN_NV04(push, NV30_3D(FP_CONTROL), 1);
      PUSH_DATA (push, fp->fp_control);
      if (eng3d->oclass < NV40_3D_CLASS) {
         BEGIN_NV04(push, NV30_3D(FP_REG_CONTROL), 1);
         PUSH_DATA (push, 0x00010004);
         BEGIN_NV04(push, NV30_3D(TEX_UNITS_ENABLE), 1);
         PUSH_DATA (push, fp->texcoords);
      } else {
         BEGIN_NV04(push, SUBC_3D(0x0b40), 1);
         PUSH_DATA (push, 0x00000000);
      }

      nv30->state.fragprog = fp;
   }
}

static void *
nv30_fp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   struct nv30_fragprog *fp = CALLOC_STRUCT(nv30_fragprog);
   if (!fp)
      return NULL;

   fp->pipe.tokens = tgsi_dup_tokens(cso->tokens);
   tgsi_scan_shader(fp->pipe.tokens, &fp->info);
   return fp;
}

static void
nv30_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
   struct nv30_fragprog *fp = hwcso;

   pipe_resource_reference(&fp->buffer, NULL);

   FREE((void *)fp->pipe.tokens);
   FREE(fp->insn);
   FREE(fp);
}

static void
nv30_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nv30_context *nv30 = nv30_context(pipe);

   nv30->fragprog.program = hwcso;
   nv30->dirty |= NV30_NEW_FRAGPROG;
}

void
nv30_fragprog_init(struct pipe_context *pipe)
{
   pipe->create_fs_state = nv30_fp_state_create;
   pipe->bind_fs_state = nv30_fp_state_bind;
   pipe->delete_fs_state = nv30_fp_state_delete;
}
