/* Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_display_color_profile_private.idl,
 *   modified Mon Dec 16 20:53:23 2013.
 */

#ifndef PPAPI_C_PRIVATE_PPB_DISPLAY_COLOR_PROFILE_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_DISPLAY_COLOR_PROFILE_PRIVATE_H_

#include "ppapi/c/pp_array_output.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_DISPLAYCOLORPROFILE_PRIVATE_INTERFACE_0_1 \
    "PPB_DisplayColorProfile_Private;0.1"
#define PPB_DISPLAYCOLORPROFILE_PRIVATE_INTERFACE \
    PPB_DISPLAYCOLORPROFILE_PRIVATE_INTERFACE_0_1

/**
 * @file
 * This file defines the <code>PPB_DisplayColorProfile</code> struct used for
 * getting the color profile of the display.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * <code>PPB_DisplayColorProfile_Private</code> defines the methods for getting
 * the display color profile and monitoring its changes.
 *
 * <strong>Setup:<strong>
 * @code
 * PP_ArrayOutput output = { MyAllocatorFunction, color_profile_data };
 * PP_Resource display_cp = display_cp_interface->Create(instance);
 * display_cp_interface->GetColorProfile(display_cp,
 *                                       output,
 *                                       completion_callback);
 * @endcode
 */
struct PPB_DisplayColorProfile_Private_0_1 {
  /**
   * Create() creates a display color profile resource.
   *
   * @param[in] instance The module instance.
   * @return A <code>PP_Resource</code> containing a display color profile
   * resource.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * IsDisplayColorProfile() determines if the given resource is a valid
   * <code>DisplayColorProfile</code> resource.
   *
   * @param[in] resource A <code>DisplayColorProfile</code> context resource.
   * @return Returns:
   * - <code>PP_TRUE</code> if the given resource is a valid
   *   <code>DisplayColorProfile</code>
   * - <code>PP_FALSE</code> if it is an invalid resource or is a resource
   *   of another type.
   */
  PP_Bool (*IsDisplayColorProfile)(PP_Resource resource);
  /**
   * GetColorProfile() enqueues a request for the current display color profile.
   *
   * This method is intended for getting the color profile data of the display
   * on which the browser window resides. [However currently Chrome only
   * considers the system's primary display color profile when doing its color
   * management. For consistency this method will also return the color profile
   * that Chrome uses for its browser window.]
   *
   * @param[in] display_color_profile_res The display color profile resource.
   * @param[in] color_profile A <code>PP_OutputArray</code> which on success
   * will receive a byte array containing the ICC color profile data (see
   * www.color.org for a reference to the ICC color profile specification
   * and versions). The returned color profile version is the one supported by
   * the host system.
   * @param[in] callback The completion callback to be called once the display
   * color profile data is available.
   *
   * @return Returns an error code from <code>pp_errors.h</code>.
   */
  int32_t (*GetColorProfile)(PP_Resource display_color_profile_res,
                             struct PP_ArrayOutput color_profile,
                             struct PP_CompletionCallback callback);
  /**
   * RegisterColorProfileChangeCallback() registers a callback to be called next
   * time the color profile for the browser window in which the plugin resides
   * changes. In order to get notifications for all color profile changes a call
   * to RegisterColorProfileChangeCallback() function should be done when the
   * previous notification was fired.
   *
   * There might be 2 scenarios in which the color profile for a window changes:
   * a) The window is moved from one display to another;
   * b) The user changes the display color space from the system settings.
   *
   * @param[in] display_color_profile_res The display color profile resource.
   * @param[in] callback The callback to be invoked next time the display
   * color profile changes.
   *
   * @return Returns an error code from <code>pp_errors.h</code>.
   */
  int32_t (*RegisterColorProfileChangeCallback)(
      PP_Resource display_color_profile_res,
      struct PP_CompletionCallback callback);
};

typedef struct PPB_DisplayColorProfile_Private_0_1
    PPB_DisplayColorProfile_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_DISPLAY_COLOR_PROFILE_PRIVATE_H_ */

