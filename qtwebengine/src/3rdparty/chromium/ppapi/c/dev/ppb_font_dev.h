/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From dev/ppb_font_dev.idl modified Thu Mar 28 10:56:39 2013. */

#ifndef PPAPI_C_DEV_PPB_FONT_DEV_H_
#define PPAPI_C_DEV_PPB_FONT_DEV_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_FONT_DEV_INTERFACE_0_6 "PPB_Font(Dev);0.6"
#define PPB_FONT_DEV_INTERFACE PPB_FONT_DEV_INTERFACE_0_6

/**
 * @file
 * This file defines the <code>PPB_Font_Dev</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
typedef enum {
  /**
   * Uses the user's default web page font (normally either the default serif
   * or sans serif font).
   */
  PP_FONTFAMILY_DEFAULT = 0,
  /**
   * These families will use the default web page font corresponding to the
   * given family.
   */
  PP_FONTFAMILY_SERIF = 1,
  PP_FONTFAMILY_SANSSERIF = 2,
  PP_FONTFAMILY_MONOSPACE = 3
} PP_FontFamily_Dev;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_FontFamily_Dev, 4);

/**
 * Specifies the font weight. Normally users will only use NORMAL or BOLD.
 */
typedef enum {
  PP_FONTWEIGHT_100 = 0,
  PP_FONTWEIGHT_200 = 1,
  PP_FONTWEIGHT_300 = 2,
  PP_FONTWEIGHT_400 = 3,
  PP_FONTWEIGHT_500 = 4,
  PP_FONTWEIGHT_600 = 5,
  PP_FONTWEIGHT_700 = 6,
  PP_FONTWEIGHT_800 = 7,
  PP_FONTWEIGHT_900 = 8,
  PP_FONTWEIGHT_NORMAL = PP_FONTWEIGHT_400,
  PP_FONTWEIGHT_BOLD = PP_FONTWEIGHT_700
} PP_FontWeight_Dev;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_FontWeight_Dev, 4);
/**
 * @}
 */

/**
 * @addtogroup Structs
 * @{
 */
struct PP_FontDescription_Dev {
  /**
   * Font face name as a string. This can also be an undefined var, in which
   * case the generic family will be obeyed. If the face is not available on
   * the system, the browser will attempt to do font fallback or pick a default
   * font.
   */
  struct PP_Var face;
  /**
   * When Create()ing a font and the face is an undefined var, the family
   * specifies the generic font family type to use. If the face is specified,
   * this will be ignored.
   *
   * When Describe()ing a font, the family will be the value you passed in when
   * the font was created. In other words, if you specify a face name, the
   * family will not be updated to reflect whether the font name you requested
   * is serif or sans serif.
   */
  PP_FontFamily_Dev family;
  /**
   * Size in pixels.
   *
   * You can specify 0 to get the default font size. The default font size
   * may vary depending on the requested font. The typical example is that
   * the user may have a different font size for the default monospace font to
   * give it a similar optical size to the proportionally spaced fonts.
   */
  uint32_t size;
  /**
   * Normally you will use either PP_FONTWEIGHT_NORMAL or PP_FONTWEIGHT_BOLD.
   */
  PP_FontWeight_Dev weight;
  PP_Bool italic;
  PP_Bool small_caps;
  /**
   * Adjustment to apply to letter and word spacing, respectively. Initialize
   * to 0 to get normal spacing. Negative values bring letters/words closer
   * together, positive values separate them.
   */
  int32_t letter_spacing;
  int32_t word_spacing;
  /**
   * Ensure that this struct is 48-bytes wide by padding the end.  In some
   * compilers, PP_Var is 8-byte aligned, so those compilers align this struct
   * on 8-byte boundaries as well and pad it to 16 bytes even without this
   * padding attribute.  This padding makes its size consistent across
   * compilers.
   */
  int32_t padding;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_FontDescription_Dev, 48);

struct PP_FontMetrics_Dev {
  int32_t height;
  int32_t ascent;
  int32_t descent;
  int32_t line_spacing;
  int32_t x_height;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_FontMetrics_Dev, 20);

struct PP_TextRun_Dev {
  /**
   * This var must either be a string or a null/undefined var (which will be
   * treated as a 0-length string).
   */
  struct PP_Var text;
  /**
   * Set to PP_TRUE if the text is right-to-left.
   *
   * When <code>override_direction</code> is false, the browser will perform
   * the Unicode Bidirectional Algorithm (http://unicode.org/reports/tr9/) on
   * the text. The value of the <code>rtl</code> flag specifies the
   * directionality of the surrounding environment. This means that Hebrew
   * word will always display right to left, even if <code>rtl</code> is false.
   *
   * When <code>override_direction</code> is true, no autodetection will be done
   * and <code>rtl</code> specifies the direction of the text.
   *
   * TODO(brettw) note that autodetection with rtl = true is currently
   * unimplemented.
   */
  PP_Bool rtl;
  /**
   * Set to PP_TRUE to force the directionality of the text regardless of
   * content.
   *
   * If this flag is set, the browser will skip autodetection of the content
   * and will display all text in the direction specified by the
   * <code>rtl</code> flag.
   */
  PP_Bool override_direction;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_TextRun_Dev, 24);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
struct PPB_Font_Dev_0_6 {
  /**
   * Returns a list of all available font families on the system. You can use
   * this list to decide whether to Create() a font.
   *
   * The return value will be a single string with null characters delimiting
   * the end of each font name. For example: "Arial\0Courier\0Times\0".
   *
   * Returns an undefined var on failure (this typically means you passed an
   * invalid instance).
   */
  struct PP_Var (*GetFontFamilies)(PP_Instance instance);
  /**
   * Returns a font which best matches the given description. The return value
   * will have a non-zero ID on success, or zero on failure.
   */
  PP_Resource (*Create)(PP_Instance instance,
                        const struct PP_FontDescription_Dev* description);
  /**
   * Returns PP_TRUE if the given resource is a Font. Returns PP_FALSE if the
   * resource is invalid or some type other than a Font.
   */
  PP_Bool (*IsFont)(PP_Resource resource);
  /**
   * Loads the description and metrics of the font into the given structures.
   * The description will be different than the description the font was
   * created with since it will be filled with the real values from the font
   * that was actually selected.
   *
   * The PP_Var in the description should be of type Void on input. On output,
   * this will contain the string and will have a reference count of 1. The
   * plugin is responsible for calling Release on this var.
   *
   * Returns PP_TRUE on success, PP_FALSE if the font is invalid or if the Var
   * in the description isn't Null (to prevent leaks).
   */
  PP_Bool (*Describe)(PP_Resource font,
                      struct PP_FontDescription_Dev* description,
                      struct PP_FontMetrics_Dev* metrics);
  /**
   * Draws the text to the image buffer.
   *
   * The given point represents the baseline of the left edge of the font,
   * regardless of whether it is left-to-right or right-to-left (in the case of
   * RTL text, this will actually represent the logical end of the text).
   *
   * The clip is optional and may be NULL. In this case, the text will be
   * clipped to the image.
   *
   * The image_data_is_opaque flag indicates whether subpixel antialiasing can
   * be performed, if it is supported. When the image below the text is
   * opaque, subpixel antialiasing is supported and you should set this to
   * PP_TRUE to pick up the user's default preferences. If your plugin is
   * partially transparent, then subpixel antialiasing is not possible and
   * grayscale antialiasing will be used instead (assuming the user has
   * antialiasing enabled at all).
   */
  PP_Bool (*DrawTextAt)(PP_Resource font,
                        PP_Resource image_data,
                        const struct PP_TextRun_Dev* text,
                        const struct PP_Point* position,
                        uint32_t color,
                        const struct PP_Rect* clip,
                        PP_Bool image_data_is_opaque);
  /**
   * Returns the width of the given string. If the font is invalid or the var
   * isn't a valid string, this will return -1.
   *
   * Note that this function handles complex scripts such as Arabic, combining
   * accents, etc. so that adding the width of substrings won't necessarily
   * produce the correct width of the entire string.
   *
   * Returns -1 on failure.
   */
  int32_t (*MeasureText)(PP_Resource font, const struct PP_TextRun_Dev* text);
  /**
   * Returns the character at the given pixel X position from the beginning of
   * the string. This handles complex scripts such as Arabic, where characters
   * may be combined or replaced depending on the context. Returns (uint32)-1
   * on failure.
   */
  uint32_t (*CharacterOffsetForPixel)(PP_Resource font,
                                      const struct PP_TextRun_Dev* text,
                                      int32_t pixel_position);
  /**
   * Returns the horizontal advance to the given character if the string was
   * placed at the given position. This handles complex scripts such as Arabic,
   * where characters may be combined or replaced depending on context. Returns
   * -1 on error.
   */
  int32_t (*PixelOffsetForCharacter)(PP_Resource font,
                                     const struct PP_TextRun_Dev* text,
                                     uint32_t char_offset);
};

typedef struct PPB_Font_Dev_0_6 PPB_Font_Dev;
/**
 * @}
 */

#endif  /* PPAPI_C_DEV_PPB_FONT_DEV_H_ */

