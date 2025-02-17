// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cocoa/cocoa_base_utils.h"

#import <objc/objc-class.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/events/event_constants.h"
#import "ui/events/test/cocoa_test_event_utils.h"
#import "ui/gfx/test/ui_cocoa_test_helper.h"

// We provide a donor class with a specially modified |modifierFlags|
// implementation that we swap with NSEvent's. This is because we can't create a
// NSEvent that represents a middle click with modifiers.
@interface TestEvent : NSObject
@end
@implementation TestEvent
- (NSUInteger)modifierFlags { return NSShiftKeyMask; }
@end

namespace ui {

namespace {

class CocoaBaseUtilsTest : public CocoaTest {
};

TEST_F(CocoaBaseUtilsTest, WindowOpenDispositionFromNSEvent) {
  // Left Click = same tab.
  NSEvent* me = cocoa_test_event_utils::MouseEventWithType(NSLeftMouseUp, 0);
  EXPECT_EQ(CURRENT_TAB, WindowOpenDispositionFromNSEvent(me));

  // Middle Click = new background tab.
  me = cocoa_test_event_utils::MouseEventWithType(NSOtherMouseUp, 0);
  EXPECT_EQ(NEW_BACKGROUND_TAB, WindowOpenDispositionFromNSEvent(me));

  // Shift+Middle Click = new foreground tab.
  {
    ScopedClassSwizzler swizzler([NSEvent class], [TestEvent class],
                                 @selector(modifierFlags));
    me = cocoa_test_event_utils::MouseEventWithType(NSOtherMouseUp,
                                                    NSShiftKeyMask);
    EXPECT_EQ(NEW_FOREGROUND_TAB, WindowOpenDispositionFromNSEvent(me));
  }

  // Cmd+Left Click = new background tab.
  me = cocoa_test_event_utils::MouseEventWithType(NSLeftMouseUp,
                                                  NSCommandKeyMask);
  EXPECT_EQ(NEW_BACKGROUND_TAB, WindowOpenDispositionFromNSEvent(me));

  // Cmd+Shift+Left Click = new foreground tab.
  me = cocoa_test_event_utils::MouseEventWithType(NSLeftMouseUp,
                                              NSCommandKeyMask |
                                              NSShiftKeyMask);
  EXPECT_EQ(NEW_FOREGROUND_TAB, WindowOpenDispositionFromNSEvent(me));

  // Shift+Left Click = new window
  me = cocoa_test_event_utils::MouseEventWithType(NSLeftMouseUp,
                                                  NSShiftKeyMask);
  EXPECT_EQ(NEW_WINDOW, WindowOpenDispositionFromNSEvent(me));
}

}  // namespace

}  // namespace ui
