// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_L10N_L10N_UTIL_MAC_H_
#define UI_BASE_L10N_L10N_UTIL_MAC_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"

#ifdef __OBJC__
@class NSString;
#else
class NSString;
#endif

namespace l10n_util {

// Remove the Windows-style accelerator marker (for labels, menuitems, etc.)
// and change "..." into an ellipsis.
// Returns the result in an autoreleased NSString.
UI_BASE_EXPORT NSString* FixUpWindowsStyleLabel(const base::string16& label);

// Pulls resource string from the string bundle and returns it.
UI_BASE_EXPORT NSString* GetNSString(int message_id);

// Get a resource string and replace $1-$2-$3 with |a| and |b|
// respectively.  Additionally, $$ is replaced by $.
UI_BASE_EXPORT NSString* GetNSStringF(int message_id, const base::string16& a);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      const base::string16& c);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      const base::string16& c,
                                      const base::string16& d);

// Variants that return the offset(s) of the replaced parameters. (See
// app/l10n_util.h for more details.)
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      std::vector<size_t>* offsets);

// Same as GetNSString, but runs the result through FixUpWindowsStyleLabel
// before returning it.
UI_BASE_EXPORT NSString* GetNSStringWithFixup(int message_id);

// Same as GetNSStringF, but runs the result through FixUpWindowsStyleLabel
// before returning it.
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a);
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a,
                                               const base::string16& b);
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a,
                                               const base::string16& b,
                                               const base::string16& c);
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a,
                                               const base::string16& b,
                                               const base::string16& c,
                                               const base::string16& d);

// Support the override of the locale with the value from Cocoa.
UI_BASE_EXPORT void OverrideLocaleWithCocoaLocale();
UI_BASE_EXPORT const std::string& GetLocaleOverride();

}  // namespace l10n_util

#endif  // UI_BASE_L10N_L10N_UTIL_MAC_H_
