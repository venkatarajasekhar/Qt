// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_KEYCODES_KEYBOARD_CODE_CONVERSION_WIN_H_
#define UI_EVENTS_KEYCODES_KEYBOARD_CODE_CONVERSION_WIN_H_

#include "ui/events/events_base_export.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ui {

// Methods to convert ui::KeyboardCode/Windows virtual key type methods.
EVENTS_BASE_EXPORT WORD WindowsKeyCodeForKeyboardCode(KeyboardCode keycode);
EVENTS_BASE_EXPORT KeyboardCode KeyboardCodeForWindowsKeyCode(WORD keycode);
EVENTS_BASE_EXPORT const char* CodeForWindowsScanCode(WORD scan_code);

}  // namespace ui

#endif  // UI_EVENTS_KEYCODES_KEYBOARD_CODE_CONVERSION_WIN_H_
