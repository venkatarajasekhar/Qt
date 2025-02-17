// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/x11_topmost_window_finder.h"

#include <algorithm>
#include <vector>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xregion.h>

// Get rid of X11 macros which conflict with gtest.
#undef Bool
#undef None

#include "base/memory/scoped_ptr.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/path.h"
#include "ui/gfx/path_x11.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/test/x11_property_change_waiter.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/desktop_aura/x11_desktop_handler.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

// Waits till |window| is minimized.
class MinimizeWaiter : public X11PropertyChangeWaiter {
 public:
  explicit MinimizeWaiter(XID window)
      : X11PropertyChangeWaiter(window, "_NET_WM_STATE") {
    const char* kAtomsToCache[] = { "_NET_WM_STATE_HIDDEN", NULL };
    atom_cache_.reset(new ui::X11AtomCache(gfx::GetXDisplay(), kAtomsToCache));
  }

  virtual ~MinimizeWaiter() {
  }

 private:
  // X11PropertyChangeWaiter:
  virtual bool ShouldKeepOnWaiting(const ui::PlatformEvent& event) OVERRIDE {
    std::vector<Atom> wm_states;
    if (ui::GetAtomArrayProperty(xwindow(), "_NET_WM_STATE", &wm_states)) {
      std::vector<Atom>::iterator it = std::find(
          wm_states.begin(),
          wm_states.end(),
          atom_cache_->GetAtom("_NET_WM_STATE_HIDDEN"));
      return it == wm_states.end();
    }
    return true;
  }

  scoped_ptr<ui::X11AtomCache> atom_cache_;

  DISALLOW_COPY_AND_ASSIGN(MinimizeWaiter);
};

// Waits till |_NET_CLIENT_LIST_STACKING| is updated to include
// |expected_windows|.
class StackingClientListWaiter : public X11PropertyChangeWaiter {
 public:
  StackingClientListWaiter(XID* expected_windows, size_t count)
      : X11PropertyChangeWaiter(ui::GetX11RootWindow(),
                                "_NET_CLIENT_LIST_STACKING"),
        expected_windows_(expected_windows, expected_windows + count) {
  }

  virtual ~StackingClientListWaiter() {
  }

  // X11PropertyChangeWaiter:
  virtual void Wait() OVERRIDE {
    // StackingClientListWaiter may be created after
    // _NET_CLIENT_LIST_STACKING already contains |expected_windows|.
    if (!ShouldKeepOnWaiting(NULL))
      return;

    X11PropertyChangeWaiter::Wait();
  }

 private:
  // X11PropertyChangeWaiter:
  virtual bool ShouldKeepOnWaiting(const ui::PlatformEvent& event) OVERRIDE {
    std::vector<XID> stack;
    ui::GetXWindowStack(ui::GetX11RootWindow(), &stack);
    for (size_t i = 0; i < expected_windows_.size(); ++i) {
      std::vector<XID>::iterator it = std::find(
          stack.begin(), stack.end(), expected_windows_[i]);
      if (it == stack.end())
        return true;
    }
    return false;
  }

  std::vector<XID> expected_windows_;

  DISALLOW_COPY_AND_ASSIGN(StackingClientListWaiter);
};

}  // namespace

class X11TopmostWindowFinderTest : public ViewsTestBase {
 public:
  X11TopmostWindowFinderTest() {
  }

  virtual ~X11TopmostWindowFinderTest() {
  }

  // Creates and shows a Widget with |bounds|. The caller takes ownership of
  // the returned widget.
  scoped_ptr<Widget> CreateAndShowWidget(const gfx::Rect& bounds) {
    scoped_ptr<Widget> toplevel(new Widget);
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_WINDOW);
    params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.native_widget = new DesktopNativeWidgetAura(toplevel.get());
    params.bounds = bounds;
    params.remove_standard_frame = true;
    toplevel->Init(params);
    toplevel->Show();
    return toplevel.Pass();
  }

  // Creates and shows an X window with |bounds|.
  XID CreateAndShowXWindow(const gfx::Rect& bounds) {
    XID root = DefaultRootWindow(xdisplay());
    XID xid = XCreateSimpleWindow(xdisplay(),
                                  root,
                                  0, 0, 1, 1,
                                  0,   // border_width
                                  0,   // border
                                  0);  // background

    ui::SetUseOSWindowFrame(xid, false);
    ShowAndSetXWindowBounds(xid, bounds);
    return xid;
  }

  // Shows |xid| and sets its bounds.
  void ShowAndSetXWindowBounds(XID xid, const gfx::Rect& bounds) {
    XMapWindow(xdisplay(), xid);

    XWindowChanges changes = {0};
    changes.x = bounds.x();
    changes.y = bounds.y();
    changes.width = bounds.width();
    changes.height = bounds.height();
    XConfigureWindow(xdisplay(),
                     xid,
                     CWX | CWY | CWWidth | CWHeight,
                     &changes);
  }

  Display* xdisplay() {
    return gfx::GetXDisplay();
  }

  // Returns the topmost X window at the passed in screen position.
  XID FindTopmostXWindowAt(int screen_x, int screen_y) {
    X11TopmostWindowFinder finder;
    return finder.FindWindowAt(gfx::Point(screen_x, screen_y));
  }

  // Returns the topmost aura::Window at the passed in screen position. Returns
  // NULL if the topmost window does not have an associated aura::Window.
  aura::Window* FindTopmostLocalProcessWindowAt(int screen_x, int screen_y) {
    X11TopmostWindowFinder finder;
    return finder.FindLocalProcessWindowAt(gfx::Point(screen_x, screen_y),
                                           std::set<aura::Window*>());
  }

  // Returns the topmost aura::Window at the passed in screen position ignoring
  // |ignore_window|. Returns NULL if the topmost window does not have an
  // associated aura::Window.
  aura::Window* FindTopmostLocalProcessWindowWithIgnore(
      int screen_x,
      int screen_y,
      aura::Window* ignore_window) {
    std::set<aura::Window*> ignore;
    ignore.insert(ignore_window);
    X11TopmostWindowFinder finder;
    return finder.FindLocalProcessWindowAt(gfx::Point(screen_x, screen_y),
                                           ignore);
  }

  // ViewsTestBase:
  virtual void SetUp() OVERRIDE {
    ViewsTestBase::SetUp();

    // Make X11 synchronous for our display connection. This does not force the
    // window manager to behave synchronously.
    XSynchronize(xdisplay(), True);

    // Ensure that the X11DesktopHandler exists. The X11DesktopHandler is
    // necessary to properly track menu windows.
    X11DesktopHandler::get();
  }

  virtual void TearDown() OVERRIDE {
    XSynchronize(xdisplay(), False);
    ViewsTestBase::TearDown();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(X11TopmostWindowFinderTest);
};

TEST_F(X11TopmostWindowFinderTest, Basic) {
  // Avoid positioning test windows at 0x0 because window managers often have a
  // panel/launcher along one of the screen edges and do not allow windows to
  // position themselves to overlap the panel/launcher.
  scoped_ptr<Widget> widget1(
      CreateAndShowWidget(gfx::Rect(100, 100, 200, 100)));
  aura::Window* window1 = widget1->GetNativeWindow();
  XID xid1 = window1->GetHost()->GetAcceleratedWidget();

  XID xid2 = CreateAndShowXWindow(gfx::Rect(200, 100, 100, 200));

  scoped_ptr<Widget> widget3(
      CreateAndShowWidget(gfx::Rect(100, 190, 200, 110)));
  aura::Window* window3 = widget3->GetNativeWindow();
  XID xid3 = window3->GetHost()->GetAcceleratedWidget();

  XID xids[] = { xid1, xid2, xid3 };
  StackingClientListWaiter waiter(xids, arraysize(xids));
  waiter.Wait();
  ui::X11EventSource::GetInstance()->DispatchXEvents();

  EXPECT_EQ(xid1, FindTopmostXWindowAt(150, 150));
  EXPECT_EQ(window1, FindTopmostLocalProcessWindowAt(150, 150));

  EXPECT_EQ(xid2, FindTopmostXWindowAt(250, 150));
  EXPECT_EQ(NULL, FindTopmostLocalProcessWindowAt(250, 150));

  EXPECT_EQ(xid3, FindTopmostXWindowAt(250, 250));
  EXPECT_EQ(window3, FindTopmostLocalProcessWindowAt(250, 250));

  EXPECT_EQ(xid3, FindTopmostXWindowAt(150, 250));
  EXPECT_EQ(window3, FindTopmostLocalProcessWindowAt(150, 250));

  EXPECT_EQ(xid3, FindTopmostXWindowAt(150, 195));
  EXPECT_EQ(window3, FindTopmostLocalProcessWindowAt(150, 195));

  EXPECT_NE(xid1, FindTopmostXWindowAt(1000, 1000));
  EXPECT_NE(xid2, FindTopmostXWindowAt(1000, 1000));
  EXPECT_NE(xid3, FindTopmostXWindowAt(1000, 1000));
  EXPECT_EQ(NULL, FindTopmostLocalProcessWindowAt(1000, 1000));

  EXPECT_EQ(window1,
            FindTopmostLocalProcessWindowWithIgnore(150, 150, window3));
  EXPECT_EQ(NULL,
            FindTopmostLocalProcessWindowWithIgnore(250, 250, window3));
  EXPECT_EQ(NULL,
            FindTopmostLocalProcessWindowWithIgnore(150, 250, window3));
  EXPECT_EQ(window1,
            FindTopmostLocalProcessWindowWithIgnore(150, 195, window3));

  XDestroyWindow(xdisplay(), xid2);
}

// Test that the minimized state is properly handled.
TEST_F(X11TopmostWindowFinderTest, Minimized) {
  scoped_ptr<Widget> widget1(
      CreateAndShowWidget(gfx::Rect(100, 100, 100, 100)));
  aura::Window* window1 = widget1->GetNativeWindow();
  XID xid1 = window1->GetHost()->GetAcceleratedWidget();
  XID xid2 = CreateAndShowXWindow(gfx::Rect(300, 100, 100, 100));

  XID xids[] = { xid1, xid2 };
  StackingClientListWaiter stack_waiter(xids, arraysize(xids));
  stack_waiter.Wait();
  ui::X11EventSource::GetInstance()->DispatchXEvents();

  EXPECT_EQ(xid1, FindTopmostXWindowAt(150, 150));
  {
    MinimizeWaiter minimize_waiter(xid1);
    XIconifyWindow(xdisplay(), xid1, 0);
    minimize_waiter.Wait();
  }
  EXPECT_NE(xid1, FindTopmostXWindowAt(150, 150));
  EXPECT_NE(xid2, FindTopmostXWindowAt(150, 150));

  // Repeat test for an X window which does not belong to a views::Widget
  // because the code path is different.
  EXPECT_EQ(xid2, FindTopmostXWindowAt(350, 150));
  {
    MinimizeWaiter minimize_waiter(xid2);
    XIconifyWindow(xdisplay(), xid2, 0);
    minimize_waiter.Wait();
  }
  EXPECT_NE(xid1, FindTopmostXWindowAt(350, 150));
  EXPECT_NE(xid2, FindTopmostXWindowAt(350, 150));

  XDestroyWindow(xdisplay(), xid2);
}

// Test that non-rectangular windows are properly handled.
TEST_F(X11TopmostWindowFinderTest, NonRectangular) {
  if (!ui::IsShapeExtensionAvailable())
    return;

  scoped_ptr<Widget> widget1(
      CreateAndShowWidget(gfx::Rect(100, 100, 100, 100)));
  XID xid1 = widget1->GetNativeWindow()->GetHost()->GetAcceleratedWidget();
  SkRegion* skregion1 = new SkRegion;
  skregion1->op(SkIRect::MakeXYWH(0, 10, 10, 90), SkRegion::kUnion_Op);
  skregion1->op(SkIRect::MakeXYWH(10, 0, 90, 100), SkRegion::kUnion_Op);
  // Widget takes ownership of |skregion1|.
  widget1->SetShape(skregion1);

  SkRegion skregion2;
  skregion2.op(SkIRect::MakeXYWH(0, 10, 10, 90), SkRegion::kUnion_Op);
  skregion2.op(SkIRect::MakeXYWH(10, 0, 90, 100), SkRegion::kUnion_Op);
  XID xid2 = CreateAndShowXWindow(gfx::Rect(300, 100, 100, 100));
  REGION* region2 = gfx::CreateRegionFromSkRegion(skregion2);
  XShapeCombineRegion(xdisplay(), xid2, ShapeBounding, 0, 0, region2,
      false);
  XDestroyRegion(region2);

  XID xids[] = { xid1, xid2 };
  StackingClientListWaiter stack_waiter(xids, arraysize(xids));
  stack_waiter.Wait();
  ui::X11EventSource::GetInstance()->DispatchXEvents();

  EXPECT_EQ(xid1, FindTopmostXWindowAt(105, 120));
  EXPECT_NE(xid1, FindTopmostXWindowAt(105, 105));
  EXPECT_NE(xid2, FindTopmostXWindowAt(105, 105));

  // Repeat test for an X window which does not belong to a views::Widget
  // because the code path is different.
  EXPECT_EQ(xid2, FindTopmostXWindowAt(305, 120));
  EXPECT_NE(xid1, FindTopmostXWindowAt(305, 105));
  EXPECT_NE(xid2, FindTopmostXWindowAt(305, 105));

  XDestroyWindow(xdisplay(), xid2);
}

// Test that the TopmostWindowFinder finds windows which belong to menus
// (which may or may not belong to Chrome).
TEST_F(X11TopmostWindowFinderTest, Menu) {
  XID xid = CreateAndShowXWindow(gfx::Rect(100, 100, 100, 100));

  XID root = DefaultRootWindow(xdisplay());
  XSetWindowAttributes swa;
  swa.override_redirect = True;
  XID menu_xid = XCreateWindow(xdisplay(),
                               root,
                               0, 0, 1, 1,
                               0,                  // border width
                               CopyFromParent,     // depth
                               InputOutput,
                               CopyFromParent,     // visual
                               CWOverrideRedirect,
                               &swa);
  {
    const char* kAtomsToCache[] = { "_NET_WM_WINDOW_TYPE_MENU", NULL };
    ui::X11AtomCache atom_cache(gfx::GetXDisplay(), kAtomsToCache);
    ui::SetAtomProperty(menu_xid,
                        "_NET_WM_WINDOW_TYPE",
                        "ATOM",
                        atom_cache.GetAtom("_NET_WM_WINDOW_TYPE_MENU"));
  }
  ui::SetUseOSWindowFrame(menu_xid, false);
  ShowAndSetXWindowBounds(menu_xid, gfx::Rect(140, 110, 100, 100));
  ui::X11EventSource::GetInstance()->DispatchXEvents();

  // |menu_xid| is never added to _NET_CLIENT_LIST_STACKING.
  XID xids[] = { xid };
  StackingClientListWaiter stack_waiter(xids, arraysize(xids));
  stack_waiter.Wait();

  EXPECT_EQ(xid, FindTopmostXWindowAt(110, 110));
  EXPECT_EQ(menu_xid, FindTopmostXWindowAt(150, 120));
  EXPECT_EQ(menu_xid, FindTopmostXWindowAt(210, 120));

  XDestroyWindow(xdisplay(), xid);
  XDestroyWindow(xdisplay(), menu_xid);
}

}  // namespace views
