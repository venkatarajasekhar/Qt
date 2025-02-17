// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/composition_text_util_pango.h"

#include <pango/pango-attributes.h>

#include "base/basictypes.h"
#include "base/i18n/char_iterator.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ime/composition_text.h"

namespace ui {

void ExtractCompositionTextFromGtkPreedit(const gchar* utf8_text,
                                          PangoAttrList* attrs,
                                          int cursor_position,
                                          CompositionText* composition) {
  composition->Clear();
  composition->text = base::UTF8ToUTF16(utf8_text);

  if (composition->text.empty())
    return;

  // Gtk/Pango uses character index for cursor position and byte index for
  // attribute range, but we use char16 offset for them. So we need to do
  // conversion here.
  std::vector<size_t> char16_offsets;
  size_t length = composition->text.length();
  base::i18n::UTF16CharIterator char_iterator(&composition->text);
  do {
    char16_offsets.push_back(char_iterator.array_pos());
  } while (char_iterator.Advance());

  // The text length in Unicode characters.
  int char_length = static_cast<int>(char16_offsets.size());
  // Make sure we can convert the value of |char_length| as well.
  char16_offsets.push_back(length);

  size_t cursor_offset =
      char16_offsets[std::max(0, std::min(char_length, cursor_position))];

  composition->selection = gfx::Range(cursor_offset);

  if (attrs) {
    int utf8_length = strlen(utf8_text);
    PangoAttrIterator* iter = pango_attr_list_get_iterator(attrs);

    // We only care about underline and background attributes and convert
    // background attribute into selection if possible.
    do {
      gint start, end;
      pango_attr_iterator_range(iter, &start, &end);

      start = std::min(start, utf8_length);
      end = std::min(end, utf8_length);
      if (start >= end)
        continue;

      start = g_utf8_pointer_to_offset(utf8_text, utf8_text + start);
      end = g_utf8_pointer_to_offset(utf8_text, utf8_text + end);

      // Double check, in case |utf8_text| is not a valid utf-8 string.
      start = std::min(start, char_length);
      end = std::min(end, char_length);
      if (start >= end)
        continue;

      PangoAttribute* background_attr =
          pango_attr_iterator_get(iter, PANGO_ATTR_BACKGROUND);
      PangoAttribute* underline_attr =
          pango_attr_iterator_get(iter, PANGO_ATTR_UNDERLINE);

      if (background_attr || underline_attr) {
        // Use a black thin underline by default.
        CompositionUnderline underline(char16_offsets[start],
                                       char16_offsets[end],
                                       SK_ColorBLACK,
                                       false,
                                       SK_ColorTRANSPARENT);

        // Always use thick underline for a range with background color, which
        // is usually the selection range.
        if (background_attr) {
          underline.thick = true;
          // If the cursor is at start or end of this underline, then we treat
          // it as the selection range as well, but make sure to set the cursor
          // position to the selection end.
          if (underline.start_offset == cursor_offset) {
            composition->selection.set_start(underline.end_offset);
            composition->selection.set_end(cursor_offset);
          } else if (underline.end_offset == cursor_offset) {
            composition->selection.set_start(underline.start_offset);
            composition->selection.set_end(cursor_offset);
          }
        }
        if (underline_attr) {
          int type = reinterpret_cast<PangoAttrInt*>(underline_attr)->value;
          if (type == PANGO_UNDERLINE_DOUBLE)
            underline.thick = true;
          else if (type == PANGO_UNDERLINE_ERROR)
            underline.color = SK_ColorRED;
        }
        composition->underlines.push_back(underline);
      }
    } while (pango_attr_iterator_next(iter));
    pango_attr_iterator_destroy(iter);
  }

  // Use a black thin underline by default.
  if (composition->underlines.empty()) {
    composition->underlines.push_back(CompositionUnderline(
        0, length, SK_ColorBLACK, false, SK_ColorTRANSPARENT));
  }
}

}  // namespace ui
