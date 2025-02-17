/* Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From ppb_text_input_controller.idl modified Thu Aug  1 09:30:48 2013. */

#ifndef PPAPI_C_PPB_TEXT_INPUT_CONTROLLER_H_
#define PPAPI_C_PPB_TEXT_INPUT_CONTROLLER_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_TEXTINPUTCONTROLLER_INTERFACE_1_0 "PPB_TextInputController;1.0"
#define PPB_TEXTINPUTCONTROLLER_INTERFACE PPB_TEXTINPUTCONTROLLER_INTERFACE_1_0

/**
 * @file
 * This file defines the <code>PPB_TextInputController</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
/**
 * PP_TextInput_Type is used to indicate the status of a plugin in regard to
 * text input.
 */
typedef enum {
  /**
   * Input caret is not in an editable mode, no input method shall be used.
   */
  PP_TEXTINPUT_TYPE_NONE = 0,
  /**
   * Input caret is in a normal editable mode, any input method can be used.
   */
  PP_TEXTINPUT_TYPE_TEXT = 1,
  /**
   * Input caret is in a password box, an input method may be used only if
   * it's suitable for password input.
   */
  PP_TEXTINPUT_TYPE_PASSWORD = 2,
  PP_TEXTINPUT_TYPE_SEARCH = 3,
  PP_TEXTINPUT_TYPE_EMAIL = 4,
  PP_TEXTINPUT_TYPE_NUMBER = 5,
  PP_TEXTINPUT_TYPE_TELEPHONE = 6,
  PP_TEXTINPUT_TYPE_URL = 7
} PP_TextInput_Type;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_TextInput_Type, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * <code>PPB_TextInputController</code> provides a set of functions for giving
 * hints to the browser about the text input status of plugins, and functions
 * for controlling input method editors (IMEs).
 */
struct PPB_TextInputController_1_0 {
  /**
   * Informs the browser about the current text input mode of the plugin.
   * Typical use of this information in the browser is to properly
   * display/suppress tools for supporting text inputs (such as virtual
   * keyboards in touch screen based devices, or input method editors often
   * used for composing East Asian characters).
   */
  void (*SetTextInputType)(PP_Instance instance, PP_TextInput_Type type);
  /**
   * Informs the browser about the coordinates of the text input caret area.
   * Typical use of this information in the browser is to layout IME windows
   * etc.
   */
  void (*UpdateCaretPosition)(PP_Instance instance,
                              const struct PP_Rect* caret);
  /**
   * Cancels the current composition in IME.
   */
  void (*CancelCompositionText)(PP_Instance instance);
  /**
   * Informs the browser about the current text selection and surrounding
   * text. <code>text</code> is a UTF-8 string that contains the current range
   * of text selection in the plugin. <code>caret</code> is the byte-index of
   * the caret position within <code>text</code>. <code>anchor</code> is the
   * byte-index of the anchor position (i.e., if a range of text is selected,
   * it is the other edge of selection different from <code>caret</code>. If
   * there are no selection, <code>anchor</code> is equal to <code>caret</code>.
   *
   * Typical use of this information in the browser is to enable "reconversion"
   * features of IME that puts back the already committed text into the
   * pre-commit composition state. Another use is to improve the precision
   * of suggestion of IME by taking the context into account (e.g., if the caret
   * looks to be on the beginning of a sentence, suggest capital letters in a
   * virtual keyboard).
   *
   * When the focus is not on text, call this function setting <code>text</code>
   * to an empty string and <code>caret</code> and <code>anchor</code> to zero.
   * Also, the plugin should send the empty text when it does not want to reveal
   * the selection to IME (e.g., when the surrounding text is containing
   * password text).
   */
  void (*UpdateSurroundingText)(PP_Instance instance,
                                struct PP_Var text,
                                uint32_t caret,
                                uint32_t anchor);
};

typedef struct PPB_TextInputController_1_0 PPB_TextInputController;
/**
 * @}
 */

#endif  /* PPAPI_C_PPB_TEXT_INPUT_CONTROLLER_H_ */

