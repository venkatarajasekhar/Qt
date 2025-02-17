// Copyright 2010 Google Inc. All Rights Reserved


#include "talk/base/gunit.h"
#include "talk/base/common.h"
#include "talk/base/logging.h"
#include "talk/base/win32window.h"
#include "talk/base/win32windowpicker.h"
#include "talk/base/windowpicker.h"

#ifndef WIN32
#error Only for Windows
#endif

namespace talk_base {

static const TCHAR* kVisibleWindowTitle = L"Visible Window";
static const TCHAR* kInvisibleWindowTitle = L"Invisible Window";

class Win32WindowPickerForTest : public Win32WindowPicker {
 public:
  Win32WindowPickerForTest() {
    EXPECT_TRUE(visible_window_.Create(NULL, kVisibleWindowTitle, WS_VISIBLE,
                                       0, 0, 0, 0, 0));
    EXPECT_TRUE(invisible_window_.Create(NULL, kInvisibleWindowTitle, 0,
                                         0, 0, 0, 0, 0));
  }

  ~Win32WindowPickerForTest() {
    visible_window_.Destroy();
    invisible_window_.Destroy();
  }

  virtual bool GetWindowList(WindowDescriptionList* descriptions) {
    if (!Win32WindowPicker::EnumProc(visible_window_.handle(),
                                     reinterpret_cast<LPARAM>(descriptions))) {
      return false;
    }
    if (!Win32WindowPicker::EnumProc(invisible_window_.handle(),
                                     reinterpret_cast<LPARAM>(descriptions))) {
      return false;
    }
    return true;
  }

  Win32Window* visible_window() {
    return &visible_window_;
  }

  Win32Window* invisible_window() {
    return &invisible_window_;
  }

 private:
  Win32Window visible_window_;
  Win32Window invisible_window_;
};

TEST(Win32WindowPickerTest, TestGetWindowList) {
  Win32WindowPickerForTest window_picker;
  WindowDescriptionList descriptions;
  EXPECT_TRUE(window_picker.GetWindowList(&descriptions));
  EXPECT_EQ(1, descriptions.size());
  WindowDescription desc = descriptions.front();
  EXPECT_EQ(window_picker.visible_window()->handle(), desc.id().id());
  TCHAR window_title[500];
  GetWindowText(window_picker.visible_window()->handle(), window_title,
                ARRAY_SIZE(window_title));
  EXPECT_EQ(0, wcscmp(window_title, kVisibleWindowTitle));
}

TEST(Win32WindowPickerTest, TestIsVisible) {
  Win32WindowPickerForTest window_picker;
  HWND visible_id = window_picker.visible_window()->handle();
  HWND invisible_id = window_picker.invisible_window()->handle();
  EXPECT_TRUE(window_picker.IsVisible(WindowId(visible_id)));
  EXPECT_FALSE(window_picker.IsVisible(WindowId(invisible_id)));
}

TEST(Win32WindowPickerTest, TestMoveToFront) {
  Win32WindowPickerForTest window_picker;
  HWND visible_id = window_picker.visible_window()->handle();
  HWND invisible_id = window_picker.invisible_window()->handle();

  // There are a number of condition where SetForegroundWindow might
  // fail depending on the state of the calling process. To be on the
  // safe side we doesn't expect MoveToFront to return true, just test
  // that we don't crash.
  window_picker.MoveToFront(WindowId(visible_id));
  window_picker.MoveToFront(WindowId(invisible_id));
}

}  // namespace talk_base
