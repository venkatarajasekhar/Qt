// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace {

base::LazyInstance<std::string> g_overridden_locale = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace l10n_util {

const std::string& GetLocaleOverride() {
  return g_overridden_locale.Get();
}

void OverrideLocaleWithCocoaLocale() {
  // NSBundle really should only be called on the main thread.
  DCHECK([NSThread isMainThread]);

  // Chrome really only has one concept of locale, but Mac OS X has locale and
  // language that can be set independently.  After talking with Chrome UX folks
  // (Cole), the best path from an experience point of view is to map the Mac OS
  // X language into the Chrome locale.  This way strings like "Yesterday" and
  // "Today" are in the same language as raw dates like "March 20, 1999" (Chrome
  // strings resources vs ICU generated strings).  This also makes the Mac acts
  // like other Chrome platforms.
  NSArray* languageList = [base::mac::OuterBundle() preferredLocalizations];
  NSString* firstLocale = [languageList objectAtIndex:0];
  // Mac OS X uses "_" instead of "-", so swap to get a real locale value.
  std::string locale_value =
      [[firstLocale stringByReplacingOccurrencesOfString:@"_"
                                              withString:@"-"] UTF8String];

  // On disk the "en-US" resources are just "en" (http://crbug.com/25578), so
  // the reverse mapping is done here to continue to feed Chrome the same values
  // in all cases on all platforms.  (l10n_util maps en to en-US if it gets
  // passed this on the command line)
  if (locale_value == "en")
    locale_value = "en-US";

  g_overridden_locale.Get() = locale_value;
}

// Remove the Windows-style accelerator marker and change "..." into an
// ellipsis.  Returns the result in an autoreleased NSString.
NSString* FixUpWindowsStyleLabel(const base::string16& label) {
  const base::char16 kEllipsisUTF16 = 0x2026;
  base::string16 ret;
  size_t label_len = label.length();
  ret.reserve(label_len);
  for (size_t i = 0; i < label_len; ++i) {
    base::char16 c = label[i];
    if (c == '(' && i + 3 < label_len && label[i + 1] == '&'
        && label[i + 3] == ')') {
      // Strip '(&?)' patterns which means Windows-style accelerator in some
      // non-English locales such as Japanese.
      i += 3;
    } else if (c == '&') {
      if (i + 1 < label_len && label[i + 1] == '&') {
        ret.push_back(c);
        ++i;
      }
    } else if (c == '.' && i + 2 < label_len && label[i + 1] == '.'
               && label[i + 2] == '.') {
      ret.push_back(kEllipsisUTF16);
      i += 2;
    } else {
      ret.push_back(c);
    }
  }

  return base::SysUTF16ToNSString(ret);
}

NSString* GetNSString(int message_id) {
  return base::SysUTF16ToNSString(l10n_util::GetStringUTF16(message_id));
}

NSString* GetNSStringF(int message_id,
                       const base::string16& a) {
  return base::SysUTF16ToNSString(l10n_util::GetStringFUTF16(message_id,
                                                             a));
}

NSString* GetNSStringF(int message_id,
                       const base::string16& a,
                       const base::string16& b) {
  return base::SysUTF16ToNSString(l10n_util::GetStringFUTF16(message_id,
                                                             a, b));
}

NSString* GetNSStringF(int message_id,
                       const base::string16& a,
                       const base::string16& b,
                       const base::string16& c) {
  return base::SysUTF16ToNSString(l10n_util::GetStringFUTF16(message_id,
                                                             a, b, c));
}

NSString* GetNSStringF(int message_id,
                       const base::string16& a,
                       const base::string16& b,
                       const base::string16& c,
                       const base::string16& d) {
  return base::SysUTF16ToNSString(l10n_util::GetStringFUTF16(message_id,
                                                             a, b, c, d));
}

NSString* GetNSStringF(int message_id,
                       const base::string16& a,
                       const base::string16& b,
                       std::vector<size_t>* offsets) {
  return base::SysUTF16ToNSString(l10n_util::GetStringFUTF16(message_id,
                                                             a, b, offsets));
}

NSString* GetNSStringWithFixup(int message_id) {
  return FixUpWindowsStyleLabel(l10n_util::GetStringUTF16(message_id));
}

NSString* GetNSStringFWithFixup(int message_id,
                                const base::string16& a) {
  return FixUpWindowsStyleLabel(l10n_util::GetStringFUTF16(message_id,
                                                           a));
}

NSString* GetNSStringFWithFixup(int message_id,
                                const base::string16& a,
                                const base::string16& b) {
  return FixUpWindowsStyleLabel(l10n_util::GetStringFUTF16(message_id,
                                                           a, b));
}

NSString* GetNSStringFWithFixup(int message_id,
                                const base::string16& a,
                                const base::string16& b,
                                const base::string16& c) {
  return FixUpWindowsStyleLabel(l10n_util::GetStringFUTF16(message_id,
                                                           a, b, c));
}

NSString* GetNSStringFWithFixup(int message_id,
                                const base::string16& a,
                                const base::string16& b,
                                const base::string16& c,
                                const base::string16& d) {
  return FixUpWindowsStyleLabel(l10n_util::GetStringFUTF16(message_id,
                                                           a, b, c, d));
}


}  // namespace l10n_util
