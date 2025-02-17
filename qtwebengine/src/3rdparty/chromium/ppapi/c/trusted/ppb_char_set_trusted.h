/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From trusted/ppb_char_set_trusted.idl modified Wed Feb  8 16:34:25 2012. */

#ifndef PPAPI_C_TRUSTED_PPB_CHAR_SET_TRUSTED_H_
#define PPAPI_C_TRUSTED_PPB_CHAR_SET_TRUSTED_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_CHARSET_TRUSTED_INTERFACE_1_0 "PPB_CharSet_Trusted;1.0"
#define PPB_CHARSET_TRUSTED_INTERFACE PPB_CHARSET_TRUSTED_INTERFACE_1_0

/**
 * @file
 *
 * This file defines the <code>PPB_CharSet_Trusted</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
typedef enum {
  /**
   * Causes the entire conversion to fail if an error is encountered. The
   * conversion function will return NULL.
   */
  PP_CHARSET_TRUSTED_CONVERSIONERROR_FAIL,
  /**
   * Silently skips over errors. Unrepresentable characters and input encoding
   * errors will be removed from the output.
   */
  PP_CHARSET_TRUSTED_CONVERSIONERROR_SKIP,
  /**
   * Replaces the error or unrepresentable character with a substitution
   * character. When converting to a Unicode character set (UTF-8 or UTF-16) it
   * will use the unicode "substitution character" U+FFFD. When converting to
   * another character set, the character will be charset-specific. For many
   * languages this will be the representation of the '?' character.
   */
  PP_CHARSET_TRUSTED_CONVERSIONERROR_SUBSTITUTE
} PP_CharSet_Trusted_ConversionError;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_CharSet_Trusted_ConversionError, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_CharSet_Trusted</code> interface provides functions for
 * converting between character sets.
 *
 * This inteface is provided for trusted plugins only since in Native Client it
 * would require an expensive out-of-process IPC call for each conversion,
 * which makes performance unacceptable. Native Client plugins should include
 * ICU or some other library if they need this feature.
 */
struct PPB_CharSet_Trusted_1_0 {
  /**
   * Converts the UTF-16 string pointed to by |*utf16| to an 8-bit string in
   * the specified code page. |utf16_len| is measured in UTF-16 units, not
   * bytes. This value may not be NULL.
   *
   * The given output buffer will be filled up to output_length bytes with the
   * result. output_length will be updated with the number of bytes required
   * for the given string. The output buffer may be null to just retrieve the
   * required buffer length.
   *
   * This function will return PP_FALSE if there was an error converting the
   * string and you requested PP_CHARSET_CONVERSIONERROR_FAIL, or the output
   * character set was unknown. Otherwise, it will return PP_TRUE.
   */
  PP_Bool (*UTF16ToCharSet)(const uint16_t utf16[],
                            uint32_t utf16_len,
                            const char* output_char_set,
                            PP_CharSet_Trusted_ConversionError on_error,
                            char* output_buffer,
                            uint32_t* output_length);
  /**
   * Same as UTF16ToCharSet except converts in the other direction. The input
   * is in the given charset, and the |input_len| is the number of bytes in
   * the |input| string.
   *
   * Note that the output_utf16_length is measured in UTF-16 characters.
   *
   * Since UTF16 can represent every Unicode character, the only time the
   * replacement character will be used is if the encoding in the input string
   * is incorrect.
   */
  PP_Bool (*CharSetToUTF16)(const char* input,
                            uint32_t input_len,
                            const char* input_char_set,
                            PP_CharSet_Trusted_ConversionError on_error,
                            uint16_t* output_buffer,
                            uint32_t* output_utf16_length);
  /**
   * Returns a string var representing the current multi-byte character set of
   * the current system.
   *
   * WARNING: You really shouldn't be using this function unless you're dealing
   * with legacy data. You should be using UTF-8 or UTF-16 and you don't have
   * to worry about the character sets.
   */
  struct PP_Var (*GetDefaultCharSet)(PP_Instance instance);
};

typedef struct PPB_CharSet_Trusted_1_0 PPB_CharSet_Trusted;
/**
 * @}
 */

#endif  /* PPAPI_C_TRUSTED_PPB_CHAR_SET_TRUSTED_H_ */

