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

#ifndef SVGA_DRAW_H
#define SVGA_DRAW_H

#include "pipe/p_compiler.h"

#include "svga_hw_reg.h"

struct svga_hwtnl;
struct svga_winsys_context;
struct svga_screen;
struct svga_context;
struct pipe_resource;
struct u_upload_mgr;

struct svga_hwtnl *svga_hwtnl_create( struct svga_context *svga,
                                      struct u_upload_mgr *upload_ib,
                                      struct svga_winsys_context *swc );

void svga_hwtnl_destroy( struct svga_hwtnl *hwtnl );

void svga_hwtnl_set_flatshade( struct svga_hwtnl *hwtnl,
                               boolean flatshade,
                               boolean flatshade_first );

void svga_hwtnl_set_unfilled( struct svga_hwtnl *hwtnl,
                              unsigned mode );

void svga_hwtnl_vdecl( struct svga_hwtnl *hwtnl,
                       unsigned i,
                       const SVGA3dVertexDecl *decl,
                       struct pipe_resource *vb);

void svga_hwtnl_reset_vdecl( struct svga_hwtnl *hwtnl,
                             unsigned count );


enum pipe_error 
svga_hwtnl_draw_arrays( struct svga_hwtnl *hwtnl,
                        unsigned prim, 
                        unsigned start, 
                        unsigned count);

enum pipe_error
svga_hwtnl_draw_range_elements( struct svga_hwtnl *hwtnl,
                                struct pipe_resource *indexBuffer,
                                unsigned index_size,
                                int index_bias,
                                unsigned min_index,
                                unsigned max_index,
                                unsigned prim, 
                                unsigned start, 
                                unsigned count );

boolean
svga_hwtnl_is_buffer_referred( struct svga_hwtnl *hwtnl,
                               struct pipe_resource *buffer );

enum pipe_error
svga_hwtnl_flush( struct svga_hwtnl *hwtnl );

void svga_hwtnl_set_index_bias( struct svga_hwtnl *hwtnl,
                                int index_bias);


#endif /* SVGA_DRAW_H_ */
