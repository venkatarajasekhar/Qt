// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/font_list.h"

#include <windows.h>

#include <set>

#include "base/strings/string16.h"
#include "base/values.h"

namespace content {

static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEXW* logical_font,
                                      NEWTEXTMETRICEXW* physical_font,
                                      DWORD font_type,
                                      LPARAM lparam) {
  std::set<base::string16>* font_names =
      reinterpret_cast<std::set<base::string16>*>(lparam);
  if (font_names) {
    const LOGFONTW& lf = logical_font->elfLogFont;
    if (lf.lfFaceName[0] && lf.lfFaceName[0] != '@') {
      base::string16 face_name(lf.lfFaceName);
      font_names->insert(face_name);
    }
  }
  return 1;
}

scoped_ptr<base::ListValue> GetFontList_SlowBlocking() {
  std::set<base::string16> font_names;

  LOGFONTW logfont;
  memset(&logfont, 0, sizeof(logfont));
  logfont.lfCharSet = DEFAULT_CHARSET;

  HDC hdc = ::GetDC(NULL);
  ::EnumFontFamiliesExW(hdc, &logfont, (FONTENUMPROCW)&EnumFontFamExProc,
                        (LPARAM)&font_names, 0);
  ::ReleaseDC(NULL, hdc);

  scoped_ptr<base::ListValue> font_list(new base::ListValue);
  std::set<base::string16>::iterator iter;
  for (iter = font_names.begin(); iter != font_names.end(); ++iter) {
    base::ListValue* font_item = new base::ListValue();
    font_item->Append(new base::StringValue(*iter));
    font_item->Append(new base::StringValue(*iter));
    font_list->Append(font_item);
  }
  return font_list.Pass();
}

}  // namespace content
