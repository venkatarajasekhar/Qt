/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From pp_graphics_3d.idl modified Wed Oct  3 15:55:01 2012. */

#ifndef PPAPI_C_PP_GRAPHICS_3D_H_
#define PPAPI_C_PP_GRAPHICS_3D_H_

#include "ppapi/c/pp_macros.h"

/**
 * @file
 * This file defines the <code>PP_Graphics3DAttrib</code> enumeration for use in
 * PPAPI C headers.
 */


/**
 * @addtogroup Enums
 * @{
 */
typedef enum {
  /**
   * Bits of Alpha in the color buffer.
   */
  PP_GRAPHICS3DATTRIB_ALPHA_SIZE = 0x3021,
  /**
   * Bits of Blue in the color buffer.
   */
  PP_GRAPHICS3DATTRIB_BLUE_SIZE = 0x3022,
  /**
   * Bits of Green in the color buffer.
   */
  PP_GRAPHICS3DATTRIB_GREEN_SIZE = 0x3023,
  /**
   * Bits of Red in the color buffer.
   */
  PP_GRAPHICS3DATTRIB_RED_SIZE = 0x3024,
  /**
   * Bits of Z in the depth buffer.
   */
  PP_GRAPHICS3DATTRIB_DEPTH_SIZE = 0x3025,
  /**
   * Bits of Stencil in the stencil buffer.
   */
  PP_GRAPHICS3DATTRIB_STENCIL_SIZE = 0x3026,
  /**
   * Number of samples per pixel.
   */
  PP_GRAPHICS3DATTRIB_SAMPLES = 0x3031,
  /**
   * Number of multisample buffers.
   */
  PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS = 0x3032,
  /**
   * Attrib list terminator.
   */
  PP_GRAPHICS3DATTRIB_NONE = 0x3038,
  /**
   * Height of surface in pixels.
   */
  PP_GRAPHICS3DATTRIB_HEIGHT = 0x3056,
  /**
   * Width of surface in pixels.
   */
  PP_GRAPHICS3DATTRIB_WIDTH = 0x3057,
  /**
   * Specifies the effect on the color buffer of posting a surface
   * with SwapBuffers. The initial value is chosen by the implementation.
   */
  PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR = 0x3093,
  /**
   * Indicates that color buffer contents are unaffected.
   */
  PP_GRAPHICS3DATTRIB_BUFFER_PRESERVED = 0x3094,
  /**
   * Indicates that color buffer contents may be destroyed or changed.
   */
  PP_GRAPHICS3DATTRIB_BUFFER_DESTROYED = 0x3095,
  /**
   * Specifies whether the context is intended to be low-power or
   * high-performance. The initial value is
   * PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE.
   */
  PP_GRAPHICS3DATTRIB_GPU_PREFERENCE = 0x11000,
  /**
   * The context should be low-power, and may be created on an integrated gpu.
   */
  PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_LOW_POWER = 0x11001,
  /**
   * The context may be high-power and may be created on a discrete gpu.
   */
  PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE = 0x11002
} PP_Graphics3DAttrib;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_Graphics3DAttrib, 4);
/**
 * @}
 */

#endif  /* PPAPI_C_PP_GRAPHICS_3D_H_ */

