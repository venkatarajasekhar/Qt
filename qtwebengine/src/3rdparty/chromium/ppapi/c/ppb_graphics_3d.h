/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From ppb_graphics_3d.idl modified Fri Aug 30 08:36:16 2013. */

#ifndef PPAPI_C_PPB_GRAPHICS_3D_H_
#define PPAPI_C_PPB_GRAPHICS_3D_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_GRAPHICS_3D_INTERFACE_1_0 "PPB_Graphics3D;1.0"
#define PPB_GRAPHICS_3D_INTERFACE PPB_GRAPHICS_3D_INTERFACE_1_0

/**
 * @file
 * Defines the <code>PPB_Graphics3D</code> struct representing a 3D graphics
 * context within the browser.
 */


/* Add 3D graphics enums */
#include "ppapi/c/pp_graphics_3d.h"

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * <code>PPB_Graphics3D</code> defines the interface for a 3D graphics context.
 * <strong>Example usage from plugin code:</strong>
 *
 * <strong>Setup:</strong>
 * @code
 * PP_Resource context;
 * int32_t attribs[] = {PP_GRAPHICS3DATTRIB_WIDTH, 800,
 *                      PP_GRAPHICS3DATTRIB_HEIGHT, 800,
 *                      PP_GRAPHICS3DATTRIB_NONE};
 * context = g3d->Create(instance, 0, attribs);
 * inst->BindGraphics(instance, context);
 * @endcode
 *
 * <strong>Present one frame:</strong>
 * @code
 * PP_CompletionCallback callback = {
 *   DidFinishSwappingBuffers, 0, PP_COMPLETIONCALLBACK_FLAG_NONE,
 * };
 * gles2->Clear(context, GL_COLOR_BUFFER_BIT);
 * g3d->SwapBuffers(context, callback);
 * @endcode
 *
 * <strong>Shutdown:</strong>
 * @code
 * core->ReleaseResource(context);
 * @endcode
 */
struct PPB_Graphics3D_1_0 {
  /**
   * GetAttribMaxValue() retrieves the maximum supported value for the
   * given attribute. This function may be used to check if a particular
   * attribute value is supported before attempting to create a context.
   *
   * @param[in] instance The module instance.
   * @param[in] attribute The attribute for which maximum value is queried.
   * Attributes that can be queried for include:
   * - <code>PP_GRAPHICS3DATTRIB_ALPHA_SIZE</code>
   * - <code>PP_GRAPHICS3DATTRIB_BLUE_SIZE</code>
   * - <code>PP_GRAPHICS3DATTRIB_GREEN_SIZE</code>
   * - <code>PP_GRAPHICS3DATTRIB_RED_SIZE</code>
   * - <code>PP_GRAPHICS3DATTRIB_DEPTH_SIZE</code>
   * - <code>PP_GRAPHICS3DATTRIB_STENCIL_SIZE</code>
   * - <code>PP_GRAPHICS3DATTRIB_SAMPLES</code>
   * - <code>PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS</code>
   * - <code>PP_GRAPHICS3DATTRIB_WIDTH</code>
   * - <code>PP_GRAPHICS3DATTRIB_HEIGHT</code>
   * @param[out] value The maximum supported value for <code>attribute</code>
   *
   * @return Returns <code>PP_TRUE</code> on success or the following on error:
   * - <code>PP_ERROR_BADRESOURCE</code> if <code>instance</code> is invalid
   * - <code>PP_ERROR_BADARGUMENT</code> if <code>attribute</code> is invalid
   *   or <code>value</code> is 0
   */
  int32_t (*GetAttribMaxValue)(PP_Resource instance,
                               int32_t attribute,
                               int32_t* value);
  /**
   * Create() creates and initializes a 3D rendering context.
   * The returned context is off-screen to start with. It must be attached to
   * a plugin instance using <code>PPB_Instance::BindGraphics</code> to draw
   * on the web page.
   *
   * @param[in] instance The module instance.
   *
   * @param[in] share_context The 3D context with which the created context
   * would share resources. If <code>share_context</code> is not 0, then all
   * shareable data, as defined by the client API (note that for OpenGL and
   * OpenGL ES, shareable data excludes texture objects named 0) will be shared
   * by <code>share_context<code>, all other contexts <code>share_context</code>
   * already shares with, and the newly created context. An arbitrary number of
   * <code>PPB_Graphics3D</code> can share data in this fashion.
   *
   * @param[in] attrib_list specifies a list of attributes for the context.
   * It is a list of attribute name-value pairs in which each attribute is
   * immediately followed by the corresponding desired value. The list is
   * terminated with <code>PP_GRAPHICS3DATTRIB_NONE</code>.
   * The <code>attrib_list<code> may be 0 or empty (first attribute is
   * <code>PP_GRAPHICS3DATTRIB_NONE</code>). If an attribute is not
   * specified in <code>attrib_list</code>, then the default value is used
   * (it is said to be specified implicitly).
   * Attributes for the context are chosen according to an attribute-specific
   * criteria. Attributes can be classified into two categories:
   * - AtLeast: The attribute value in the returned context meets or exceeds
   *            the value specified in <code>attrib_list</code>.
   * - Exact: The attribute value in the returned context is equal to
   *          the value specified in <code>attrib_list</code>.
   *
   * Attributes that can be specified in <code>attrib_list</code> include:
   * - <code>PP_GRAPHICS3DATTRIB_ALPHA_SIZE</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_BLUE_SIZE</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_GREEN_SIZE</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_RED_SIZE</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_DEPTH_SIZE</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_STENCIL_SIZE</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_SAMPLES</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS</code>:
   *   Category: AtLeast Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_WIDTH</code>:
   *   Category: Exact Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_HEIGHT</code>:
   *   Category: Exact Default: 0.
   * - <code>PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR</code>:
   *   Category: Exact Default: Implementation defined.
   *
   * @return A <code>PP_Resource</code> containing the 3D graphics context if
   * successful or 0 if unsuccessful.
   */
  PP_Resource (*Create)(PP_Instance instance,
                        PP_Resource share_context,
                        const int32_t attrib_list[]);
  /**
   * IsGraphics3D() determines if the given resource is a valid
   * <code>Graphics3D</code> context.
   *
   * @param[in] resource A <code>Graphics3D</code> context resource.
   *
   * @return PP_TRUE if the given resource is a valid <code>Graphics3D</code>,
   * <code>PP_FALSE</code> if it is an invalid resource or is a resource of
   * another type.
   */
  PP_Bool (*IsGraphics3D)(PP_Resource resource);
  /**
   * GetAttribs() retrieves the value for each attribute in
   * <code>attrib_list</code>.
   *
   * @param[in] context The 3D graphics context.
   * @param[in,out] attrib_list The list of attributes that are queried.
   * <code>attrib_list</code> has the same structure as described for
   * <code>PPB_Graphics3D::Create</code>. It is both input and output
   * structure for this function. All attributes specified in
   * <code>PPB_Graphics3D::Create</code> can be queried for.
   *
   * @return Returns <code>PP_OK</code> on success or:
   * - <code>PP_ERROR_BADRESOURCE</code> if context is invalid
   * - <code>PP_ERROR_BADARGUMENT</code> if attrib_list is 0 or any attribute
   *   in the <code>attrib_list</code> is not a valid attribute.
   *
   * <strong>Example usage:</strong> To get the values for rgb bits in the
   * color buffer, this function must be called as following:
   * @code
   * int attrib_list[] = {PP_GRAPHICS3DATTRIB_RED_SIZE, 0,
   *                      PP_GRAPHICS3DATTRIB_GREEN_SIZE, 0,
   *                      PP_GRAPHICS3DATTRIB_BLUE_SIZE, 0,
   *                      PP_GRAPHICS3DATTRIB_NONE};
   * GetAttribs(context, attrib_list);
   * int red_bits = attrib_list[1];
   * int green_bits = attrib_list[3];
   * int blue_bits = attrib_list[5];
   * @endcode
   */
  int32_t (*GetAttribs)(PP_Resource context, int32_t attrib_list[]);
  /**
   * SetAttribs() sets the values for each attribute in
   * <code>attrib_list</code>.
   *
   * @param[in] context The 3D graphics context.
   * @param[in] attrib_list The list of attributes whose values need to be set.
   * <code>attrib_list</code> has the same structure as described for
   * <code>PPB_Graphics3D::Create</code>.
   * Attributes that can be specified are:
   * - <code>PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR</code>
   *
   * @return Returns <code>PP_OK</code> on success or:
   * - <code>PP_ERROR_BADRESOURCE</code> if <code>context</code> is invalid.
   * - <code>PP_ERROR_BADARGUMENT</code> if <code>attrib_list</code> is 0 or
   *   any attribute in the <code>attrib_list</code> is not a valid attribute.
   */
  int32_t (*SetAttribs)(PP_Resource context, const int32_t attrib_list[]);
  /**
   * GetError() returns the current state of the given 3D context.
   *
   * The recoverable error conditions that have no side effect are
   * detected and returned immediately by all functions in this interface.
   * In addition the implementation may get into a fatal state while
   * processing a command. In this case the application must destroy the
   * context and reinitialize client API state and objects to continue
   * rendering.
   *
   * Note that the same error code is also returned in the SwapBuffers callback.
   * It is recommended to handle error in the SwapBuffers callback because
   * GetError is synchronous. This function may be useful in rare cases where
   * drawing a frame is expensive and you want to verify the result of
   * ResizeBuffers before attempting to draw a frame.
   *
   * @param[in] The 3D graphics context.
   * @return Returns:
   * - <code>PP_OK</code> if no error
   * - <code>PP_ERROR_NOMEMORY</code>
   * - <code>PP_ERROR_CONTEXT_LOST</code>
   */
  int32_t (*GetError)(PP_Resource context);
  /**
   * ResizeBuffers() resizes the backing surface for context.
   *
   * If the surface could not be resized due to insufficient resources,
   * <code>PP_ERROR_NOMEMORY</code> error is returned on the next
   * <code>SwapBuffers</code> callback.
   *
   * @param[in] context The 3D graphics context.
   * @param[in] width The width of the backing surface.
   * @param[in] height The height of the backing surface.
   * @return Returns <code>PP_OK</code> on success or:
   * - <code>PP_ERROR_BADRESOURCE</code> if context is invalid.
   * - <code>PP_ERROR_BADARGUMENT</code> if the value specified for
   *   <code>width</code> or <code>height</code> is less than zero.
   */
  int32_t (*ResizeBuffers)(PP_Resource context, int32_t width, int32_t height);
  /**
   * SwapBuffers() makes the contents of the color buffer available for
   * compositing. This function has no effect on off-screen surfaces - ones not
   * bound to any plugin instance. The contents of ancillary buffers are always
   * undefined after calling <code>SwapBuffers</code>. The contents of the color
   * buffer are undefined if the value of the
   * <code>PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR</code> attribute of context is not
   * <code>PP_GRAPHICS3DATTRIB_BUFFER_PRESERVED</code>.
   *
   * <code>SwapBuffers</code> runs in asynchronous mode. Specify a callback
   * function and the argument for that callback function. The callback function
   * will be executed on the calling thread after the color buffer has been
   * composited with rest of the html page. While you are waiting for a
   * SwapBuffers callback, additional calls to SwapBuffers will fail.
   *
   * Because the callback is executed (or thread unblocked) only when the
   * plugin's current state is actually on the screen, this function provides a
   * way to rate limit animations. By waiting until the image is on the screen
   * before painting the next frame, you can ensure you're not generating
   * updates faster than the screen can be updated.
   *
   * SwapBuffers performs an implicit flush operation on context.
   * If the context gets into an unrecoverable error condition while
   * processing a command, the error code will be returned as the argument
   * for the callback. The callback may return the following error codes:
   * - <code>PP_ERROR_NOMEMORY</code>
   * - <code>PP_ERROR_CONTEXT_LOST</code>
   * Note that the same error code may also be obtained by calling GetError.
   *
   * @param[in] context The 3D graphics context.
   * @param[in] callback The callback that will executed when
   * <code>SwapBuffers</code> completes.
   *
   * @return Returns PP_OK on success or:
   * - <code>PP_ERROR_BADRESOURCE</code> if context is invalid.
   * - <code>PP_ERROR_BADARGUMENT</code> if callback is invalid.
   *
   */
  int32_t (*SwapBuffers)(PP_Resource context,
                         struct PP_CompletionCallback callback);
};

typedef struct PPB_Graphics3D_1_0 PPB_Graphics3D;
/**
 * @}
 */

#endif  /* PPAPI_C_PPB_GRAPHICS_3D_H_ */

