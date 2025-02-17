// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/accelerators/menu_label_accelerator_util_linux.h"

#include "base/strings/string_util.h"

namespace {

// Common implementation of ConvertAcceleratorsFromWindowsStyle() and
// RemoveWindowsStyleAccelerators().
// Replaces all ampersands (as used in our grd files to indicate mnemonics)
// to |target|, except ampersands appearing in pairs which are replaced by
// a single ampersand. Any underscores get replaced with two underscores as
// is needed by GTK.
std::string ConvertAmpersandsTo(const std::string& label,
                                const std::string& target) {
  std::string ret;
  ret.reserve(label.length() * 2);
  for (size_t i = 0; i < label.length(); ++i) {
    if ('_' == label[i]) {
      ret.push_back('_');
      ret.push_back('_');
    } else if ('&' == label[i]) {
      if (i + 1 < label.length() && '&' == label[i + 1]) {
        ret.push_back('&');
        ++i;
      } else {
        ret.append(target);
      }
    } else {
      ret.push_back(label[i]);
    }
  }

  return ret;
}

}  // namespace

namespace ui {

std::string ConvertAcceleratorsFromWindowsStyle(const std::string& label) {
  return ConvertAmpersandsTo(label, "_");
}

std::string RemoveWindowsStyleAccelerators(const std::string& label) {
  return ConvertAmpersandsTo(label, std::string());
}

// Replaces all ampersands in |label| with two ampersands. This effectively
// escapes strings for later processing by ConvertAmpersandsTo(), so that
// ConvertAmpersandsTo(EscapeWindowsStyleAccelerators(x), *) is |x| with
// underscores doubled, making the string that appears to the user just |x|.
std::string EscapeWindowsStyleAccelerators(const std::string& label) {
  std::string ret;
  base::ReplaceChars(label, "&", "&&", &ret);
  return ret;
}

}  // namespace ui
