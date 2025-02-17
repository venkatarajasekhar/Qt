// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_tree_host_x11.h"

#include <strings.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>

#include <algorithm>
#include <limits>
#include <string>

#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/ui_base_switches.h"
#include "ui/base/view_prop.h"
#include "ui/base/x/x11_util.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/dip_util.h"
#include "ui/compositor/layer.h"
#include "ui/events/event.h"
#include "ui/events/event_switches.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/events/platform/platform_event_observer.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/events/x/device_data_manager.h"
#include "ui/events/x/device_list_cache_x.h"
#include "ui/events/x/touch_factory_x11.h"
#include "ui/gfx/screen.h"

using std::max;
using std::min;

namespace aura {

namespace {

const char* kAtomsToCache[] = {
  "WM_DELETE_WINDOW",
  "_NET_WM_PING",
  "_NET_WM_PID",
  NULL
};

::Window FindEventTarget(const base::NativeEvent& xev) {
  ::Window target = xev->xany.window;
  if (xev->type == GenericEvent)
    target = static_cast<XIDeviceEvent*>(xev->xcookie.data)->event;
  return target;
}

void SelectXInput2EventsForRootWindow(XDisplay* display, ::Window root_window) {
  CHECK(ui::IsXInput2Available());
  unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {};
  memset(mask, 0, sizeof(mask));

  XISetMask(mask, XI_HierarchyChanged);
  XISetMask(mask, XI_KeyPress);
  XISetMask(mask, XI_KeyRelease);

  XIEventMask evmask;
  evmask.deviceid = XIAllDevices;
  evmask.mask_len = sizeof(mask);
  evmask.mask = mask;
  XISelectEvents(display, root_window, &evmask, 1);

#if defined(OS_CHROMEOS)
  if (base::SysInfo::IsRunningOnChromeOS()) {
    // It is necessary to listen for touch events on the root window for proper
    // touch event calibration on Chrome OS, but this is not currently necessary
    // on the desktop. This seems to fail in some cases (e.g. when logging
    // in incognito). So select for non-touch events first, and then select for
    // touch-events (but keep the other events in the mask, i.e. do not memset
    // |mask| back to 0).
    // TODO(sad): Figure out why this happens. http://crbug.com/153976
    XISetMask(mask, XI_TouchBegin);
    XISetMask(mask, XI_TouchUpdate);
    XISetMask(mask, XI_TouchEnd);
    XISelectEvents(display, root_window, &evmask, 1);
  }
#endif
}

bool default_override_redirect = false;

}  // namespace

namespace internal {

// TODO(miletus) : Move this into DeviceDataManager.
// Accomplishes 2 tasks concerning touch event calibration:
// 1. Being a message-pump observer,
//    routes all the touch events to the X root window,
//    where they can be calibrated later.
// 2. Has the Calibrate method that does the actual bezel calibration,
//    when invoked from X root window's event dispatcher.
class TouchEventCalibrate : public ui::PlatformEventObserver {
 public:
  TouchEventCalibrate() : left_(0), right_(0), top_(0), bottom_(0) {
    if (ui::PlatformEventSource::GetInstance())
      ui::PlatformEventSource::GetInstance()->AddPlatformEventObserver(this);
#if defined(USE_XI2_MT)
    std::vector<std::string> parts;
    if (Tokenize(CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                     switches::kTouchCalibration),
                 ",",
                 &parts) >= 4) {
      if (!base::StringToInt(parts[0], &left_))
        DLOG(ERROR) << "Incorrect left border calibration value passed.";
      if (!base::StringToInt(parts[1], &right_))
        DLOG(ERROR) << "Incorrect right border calibration value passed.";
      if (!base::StringToInt(parts[2], &top_))
        DLOG(ERROR) << "Incorrect top border calibration value passed.";
      if (!base::StringToInt(parts[3], &bottom_))
        DLOG(ERROR) << "Incorrect bottom border calibration value passed.";
    }
#endif  // defined(USE_XI2_MT)
  }

  virtual ~TouchEventCalibrate() {
    if (ui::PlatformEventSource::GetInstance())
      ui::PlatformEventSource::GetInstance()->RemovePlatformEventObserver(this);
  }

  // Modify the location of the |event|,
  // expanding it from |bounds| to (|bounds| + bezels).
  // Required when touchscreen is bigger than screen (i.e. has bezels),
  // because we receive events in touchscreen coordinates,
  // which need to be expanded when converting to screen coordinates,
  // so that location on bezels will be outside of screen area.
  void Calibrate(ui::TouchEvent* event, const gfx::Rect& bounds) {
#if defined(USE_XI2_MT)
    int x = event->x();
    int y = event->y();

    if (!left_ && !right_ && !top_ && !bottom_)
      return;

    const int resolution_x = bounds.width();
    const int resolution_y = bounds.height();
    // The "grace area" (10% in this case) is to make it easier for the user to
    // navigate to the corner.
    const double kGraceAreaFraction = 0.1;
    if (left_ || right_) {
      // Offset the x position to the real
      x -= left_;
      // Check if we are in the grace area of the left side.
      // Note: We might not want to do this when the gesture is locked?
      if (x < 0 && x > -left_ * kGraceAreaFraction)
        x = 0;
      // Check if we are in the grace area of the right side.
      // Note: We might not want to do this when the gesture is locked?
      if (x > resolution_x - left_ &&
          x < resolution_x - left_ + right_ * kGraceAreaFraction)
        x = resolution_x - left_;
      // Scale the screen area back to the full resolution of the screen.
      x = (x * resolution_x) / (resolution_x - (right_ + left_));
    }
    if (top_ || bottom_) {
      // When there is a top bezel we add our border,
      y -= top_;

      // Check if we are in the grace area of the top side.
      // Note: We might not want to do this when the gesture is locked?
      if (y < 0 && y > -top_ * kGraceAreaFraction)
        y = 0;

      // Check if we are in the grace area of the bottom side.
      // Note: We might not want to do this when the gesture is locked?
      if (y > resolution_y - top_ &&
          y < resolution_y - top_ + bottom_ * kGraceAreaFraction)
        y = resolution_y - top_;
      // Scale the screen area back to the full resolution of the screen.
      y = (y * resolution_y) / (resolution_y - (bottom_ + top_));
    }

    // Set the modified coordinate back to the event.
    if (event->root_location() == event->location()) {
      // Usually those will be equal,
      // if not, I am not sure what the correct value should be.
      event->set_root_location(gfx::Point(x, y));
    }
    event->set_location(gfx::Point(x, y));
#endif  // defined(USE_XI2_MT)
  }

 private:
  // ui::PlatformEventObserver:
  virtual void WillProcessEvent(const ui::PlatformEvent& event) OVERRIDE {
#if defined(USE_XI2_MT)
    if (event->type == GenericEvent &&
        (event->xgeneric.evtype == XI_TouchBegin ||
         event->xgeneric.evtype == XI_TouchUpdate ||
         event->xgeneric.evtype == XI_TouchEnd)) {
      XIDeviceEvent* xievent = static_cast<XIDeviceEvent*>(event->xcookie.data);
      xievent->event = xievent->root;
      xievent->event_x = xievent->root_x;
      xievent->event_y = xievent->root_y;
    }
#endif  // defined(USE_XI2_MT)
  }

  virtual void DidProcessEvent(const ui::PlatformEvent& event) OVERRIDE {}

  // The difference in screen's native resolution pixels between
  // the border of the touchscreen and the border of the screen,
  // aka bezel sizes.
  int left_;
  int right_;
  int top_;
  int bottom_;

  DISALLOW_COPY_AND_ASSIGN(TouchEventCalibrate);
};

}  // namespace internal

////////////////////////////////////////////////////////////////////////////////
// WindowTreeHostX11

WindowTreeHostX11::WindowTreeHostX11(const gfx::Rect& bounds)
    : xdisplay_(gfx::GetXDisplay()),
      xwindow_(0),
      x_root_window_(DefaultRootWindow(xdisplay_)),
      current_cursor_(ui::kCursorNull),
      window_mapped_(false),
      bounds_(bounds),
      touch_calibrate_(new internal::TouchEventCalibrate),
      atom_cache_(xdisplay_, kAtomsToCache) {
  XSetWindowAttributes swa;
  memset(&swa, 0, sizeof(swa));
  swa.background_pixmap = None;
  swa.override_redirect = default_override_redirect;
  xwindow_ = XCreateWindow(
      xdisplay_, x_root_window_,
      bounds.x(), bounds.y(), bounds.width(), bounds.height(),
      0,               // border width
      CopyFromParent,  // depth
      InputOutput,
      CopyFromParent,  // visual
      CWBackPixmap | CWOverrideRedirect,
      &swa);
  if (ui::PlatformEventSource::GetInstance())
    ui::PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);

  long event_mask = ButtonPressMask | ButtonReleaseMask | FocusChangeMask |
                    KeyPressMask | KeyReleaseMask |
                    EnterWindowMask | LeaveWindowMask |
                    ExposureMask | VisibilityChangeMask |
                    StructureNotifyMask | PropertyChangeMask |
                    PointerMotionMask;
  XSelectInput(xdisplay_, xwindow_, event_mask);
  XFlush(xdisplay_);

  if (ui::IsXInput2Available()) {
    ui::TouchFactory::GetInstance()->SetupXI2ForXWindow(xwindow_);
    SelectXInput2EventsForRootWindow(xdisplay_, x_root_window_);
  }

  // TODO(erg): We currently only request window deletion events. We also
  // should listen for activation events and anything else that GTK+ listens
  // for, and do something useful.
  ::Atom protocols[2];
  protocols[0] = atom_cache_.GetAtom("WM_DELETE_WINDOW");
  protocols[1] = atom_cache_.GetAtom("_NET_WM_PING");
  XSetWMProtocols(xdisplay_, xwindow_, protocols, 2);

  // We need a WM_CLIENT_MACHINE and WM_LOCALE_NAME value so we integrate with
  // the desktop environment.
  XSetWMProperties(xdisplay_, xwindow_, NULL, NULL, NULL, 0, NULL, NULL, NULL);

  // Likewise, the X server needs to know this window's pid so it knows which
  // program to kill if the window hangs.
  // XChangeProperty() expects "pid" to be long.
  COMPILE_ASSERT(sizeof(long) >= sizeof(pid_t), pid_t_bigger_than_long);
  long pid = getpid();
  XChangeProperty(xdisplay_,
                  xwindow_,
                  atom_cache_.GetAtom("_NET_WM_PID"),
                  XA_CARDINAL,
                  32,
                  PropModeReplace,
                  reinterpret_cast<unsigned char*>(&pid), 1);

  // Allow subclasses to create and cache additional atoms.
  atom_cache_.allow_uncached_atoms();

  XRRSelectInput(xdisplay_, x_root_window_,
                 RRScreenChangeNotifyMask | RROutputChangeNotifyMask);
  CreateCompositor(GetAcceleratedWidget());
}

WindowTreeHostX11::~WindowTreeHostX11() {
  if (ui::PlatformEventSource::GetInstance())
    ui::PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);

  DestroyCompositor();
  DestroyDispatcher();
  XDestroyWindow(xdisplay_, xwindow_);
}

bool WindowTreeHostX11::CanDispatchEvent(const ui::PlatformEvent& event) {
  ::Window target = FindEventTarget(event);
  return target == xwindow_ || target == x_root_window_;
}

uint32_t WindowTreeHostX11::DispatchEvent(const ui::PlatformEvent& event) {
  XEvent* xev = event;
  if (FindEventTarget(xev) == x_root_window_) {
    if (xev->type == GenericEvent)
      DispatchXI2Event(xev);
    return ui::POST_DISPATCH_NONE;
  }

  switch (xev->type) {
    case EnterNotify: {
      // Ignore EventNotify events from children of |xwindow_|.
      // NativeViewGLSurfaceGLX adds a child to |xwindow_|.
      // TODO(pkotwicz|tdanderson): Figure out whether the suppression is
      // necessary. crbug.com/385716
      if (xev->xcrossing.detail == NotifyInferior)
        break;

      aura::Window* root_window = window();
      client::CursorClient* cursor_client =
          client::GetCursorClient(root_window);
      if (cursor_client) {
        const gfx::Display display = gfx::Screen::GetScreenFor(root_window)->
            GetDisplayNearestWindow(root_window);
        cursor_client->SetDisplay(display);
      }
      ui::MouseEvent mouse_event(xev);
      // EnterNotify creates ET_MOUSE_MOVE. Mark as synthesized as this is not
      // real mouse move event.
      mouse_event.set_flags(mouse_event.flags() | ui::EF_IS_SYNTHESIZED);
      TranslateAndDispatchLocatedEvent(&mouse_event);
      break;
    }
    case LeaveNotify: {
      // Ignore LeaveNotify events from children of |xwindow_|.
      // NativeViewGLSurfaceGLX adds a child to |xwindow_|.
      // TODO(pkotwicz|tdanderson): Figure out whether the suppression is
      // necessary. crbug.com/385716
      if (xev->xcrossing.detail == NotifyInferior)
        break;

      ui::MouseEvent mouse_event(xev);
      TranslateAndDispatchLocatedEvent(&mouse_event);
      break;
    }
    case Expose: {
      gfx::Rect damage_rect(xev->xexpose.x, xev->xexpose.y,
                            xev->xexpose.width, xev->xexpose.height);
      compositor()->ScheduleRedrawRect(damage_rect);
      break;
    }
    case KeyPress: {
      ui::KeyEvent keydown_event(xev, false);
      SendEventToProcessor(&keydown_event);
      break;
    }
    case KeyRelease: {
      ui::KeyEvent keyup_event(xev, false);
      SendEventToProcessor(&keyup_event);
      break;
    }
    case ButtonPress:
    case ButtonRelease: {
      switch (ui::EventTypeFromNative(xev)) {
        case ui::ET_MOUSEWHEEL: {
          ui::MouseWheelEvent mouseev(xev);
          TranslateAndDispatchLocatedEvent(&mouseev);
          break;
        }
        case ui::ET_MOUSE_PRESSED:
        case ui::ET_MOUSE_RELEASED: {
          ui::MouseEvent mouseev(xev);
          TranslateAndDispatchLocatedEvent(&mouseev);
          break;
        }
        case ui::ET_UNKNOWN:
          // No event is created for X11-release events for mouse-wheel buttons.
          break;
        default:
          NOTREACHED();
      }
      break;
    }
    case FocusOut:
      if (xev->xfocus.mode != NotifyGrab)
        OnHostLostWindowCapture();
      break;
    case ConfigureNotify: {
      DCHECK_EQ(xwindow_, xev->xconfigure.event);
      DCHECK_EQ(xwindow_, xev->xconfigure.window);
      // It's possible that the X window may be resized by some other means
      // than from within aura (e.g. the X window manager can change the
      // size). Make sure the root window size is maintained properly.
      gfx::Rect bounds(xev->xconfigure.x, xev->xconfigure.y,
          xev->xconfigure.width, xev->xconfigure.height);
      bool size_changed = bounds_.size() != bounds.size();
      bool origin_changed = bounds_.origin() != bounds.origin();
      bounds_ = bounds;
      OnConfigureNotify();
      if (size_changed)
        OnHostResized(bounds.size());
      if (origin_changed)
        OnHostMoved(bounds_.origin());
      break;
    }
    case GenericEvent:
      DispatchXI2Event(xev);
      break;
    case ClientMessage: {
      Atom message_type = static_cast<Atom>(xev->xclient.data.l[0]);
      if (message_type == atom_cache_.GetAtom("WM_DELETE_WINDOW")) {
        // We have received a close message from the window manager.
        OnHostCloseRequested();
      } else if (message_type == atom_cache_.GetAtom("_NET_WM_PING")) {
        XEvent reply_event = *xev;
        reply_event.xclient.window = x_root_window_;

        XSendEvent(xdisplay_,
                   reply_event.xclient.window,
                   False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &reply_event);
        XFlush(xdisplay_);
      }
      break;
    }
    case MappingNotify: {
      switch (xev->xmapping.request) {
        case MappingModifier:
        case MappingKeyboard:
          XRefreshKeyboardMapping(&xev->xmapping);
          break;
        case MappingPointer:
          ui::DeviceDataManager::GetInstance()->UpdateButtonMap();
          break;
        default:
          NOTIMPLEMENTED() << " Unknown request: " << xev->xmapping.request;
          break;
      }
      break;
    }
    case MotionNotify: {
      // Discard all but the most recent motion event that targets the same
      // window with unchanged state.
      XEvent last_event;
      while (XPending(xev->xany.display)) {
        XEvent next_event;
        XPeekEvent(xev->xany.display, &next_event);
        if (next_event.type == MotionNotify &&
            next_event.xmotion.window == xev->xmotion.window &&
            next_event.xmotion.subwindow == xev->xmotion.subwindow &&
            next_event.xmotion.state == xev->xmotion.state) {
          XNextEvent(xev->xany.display, &last_event);
          xev = &last_event;
        } else {
          break;
        }
      }

      ui::MouseEvent mouseev(xev);
      TranslateAndDispatchLocatedEvent(&mouseev);
      break;
    }
  }
  return ui::POST_DISPATCH_STOP_PROPAGATION;
}

ui::EventSource* WindowTreeHostX11::GetEventSource() {
  return this;
}

gfx::AcceleratedWidget WindowTreeHostX11::GetAcceleratedWidget() {
  return xwindow_;
}

void WindowTreeHostX11::Show() {
  if (!window_mapped_) {
    // Before we map the window, set size hints. Otherwise, some window managers
    // will ignore toplevel XMoveWindow commands.
    XSizeHints size_hints;
    size_hints.flags = PPosition | PWinGravity;
    size_hints.x = bounds_.x();
    size_hints.y = bounds_.y();
    // Set StaticGravity so that the window position is not affected by the
    // frame width when running with window manager.
    size_hints.win_gravity = StaticGravity;
    XSetWMNormalHints(xdisplay_, xwindow_, &size_hints);

    XMapWindow(xdisplay_, xwindow_);

    // We now block until our window is mapped. Some X11 APIs will crash and
    // burn if passed |xwindow_| before the window is mapped, and XMapWindow is
    // asynchronous.
    if (ui::X11EventSource::GetInstance())
      ui::X11EventSource::GetInstance()->BlockUntilWindowMapped(xwindow_);
    window_mapped_ = true;
  }
}

void WindowTreeHostX11::Hide() {
  if (window_mapped_) {
    XWithdrawWindow(xdisplay_, xwindow_, 0);
    window_mapped_ = false;
  }
}

gfx::Rect WindowTreeHostX11::GetBounds() const {
  return bounds_;
}

void WindowTreeHostX11::SetBounds(const gfx::Rect& bounds) {
  // Even if the host window's size doesn't change, aura's root window
  // size, which is in DIP, changes when the scale changes.
  float current_scale = compositor()->device_scale_factor();
  float new_scale = gfx::Screen::GetScreenFor(window())->
      GetDisplayNearestWindow(window()).device_scale_factor();
  bool origin_changed = bounds_.origin() != bounds.origin();
  bool size_changed = bounds_.size() != bounds.size();
  XWindowChanges changes = {0};
  unsigned value_mask = 0;

  if (size_changed) {
    changes.width = bounds.width();
    changes.height = bounds.height();
    value_mask = CWHeight | CWWidth;
  }

  if (origin_changed) {
    changes.x = bounds.x();
    changes.y = bounds.y();
    value_mask |= CWX | CWY;
  }
  if (value_mask)
    XConfigureWindow(xdisplay_, xwindow_, value_mask, &changes);

  // Assume that the resize will go through as requested, which should be the
  // case if we're running without a window manager.  If there's a window
  // manager, it can modify or ignore the request, but (per ICCCM) we'll get a
  // (possibly synthetic) ConfigureNotify about the actual size and correct
  // |bounds_| later.
  bounds_ = bounds;
  if (origin_changed)
    OnHostMoved(bounds.origin());
  if (size_changed || current_scale != new_scale) {
    OnHostResized(bounds.size());
  } else {
    window()->SchedulePaintInRect(window()->bounds());
  }
}

gfx::Point WindowTreeHostX11::GetLocationOnNativeScreen() const {
  return bounds_.origin();
}

void WindowTreeHostX11::SetCapture() {
  // TODO(oshima): Grab x input.
}

void WindowTreeHostX11::ReleaseCapture() {
  // TODO(oshima): Release x input.
}

void WindowTreeHostX11::PostNativeEvent(
    const base::NativeEvent& native_event) {
  DCHECK(xwindow_);
  DCHECK(xdisplay_);
  XEvent xevent = *native_event;
  xevent.xany.display = xdisplay_;
  xevent.xany.window = xwindow_;

  switch (xevent.type) {
    case EnterNotify:
    case LeaveNotify:
    case MotionNotify:
    case KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease: {
      // The fields used below are in the same place for all of events
      // above. Using xmotion from XEvent's unions to avoid repeating
      // the code.
      xevent.xmotion.root = x_root_window_;
      xevent.xmotion.time = CurrentTime;

      gfx::Point point(xevent.xmotion.x, xevent.xmotion.y);
      ConvertPointToNativeScreen(&point);
      xevent.xmotion.x_root = point.x();
      xevent.xmotion.y_root = point.y();
    }
    default:
      break;
  }
  XSendEvent(xdisplay_, xwindow_, False, 0, &xevent);
  XFlush(xdisplay_);
}

void WindowTreeHostX11::OnDeviceScaleFactorChanged(
    float device_scale_factor) {
}

void WindowTreeHostX11::SetCursorNative(gfx::NativeCursor cursor) {
  if (cursor == current_cursor_)
    return;
  current_cursor_ = cursor;
  SetCursorInternal(cursor);
}

void WindowTreeHostX11::MoveCursorToNative(const gfx::Point& location) {
  XWarpPointer(xdisplay_, None, x_root_window_, 0, 0, 0, 0,
               bounds_.x() + location.x(),
               bounds_.y() + location.y());
}

void WindowTreeHostX11::OnCursorVisibilityChangedNative(bool show) {
}

ui::EventProcessor* WindowTreeHostX11::GetEventProcessor() {
  return dispatcher();
}

void WindowTreeHostX11::DispatchXI2Event(const base::NativeEvent& event) {
  ui::TouchFactory* factory = ui::TouchFactory::GetInstance();
  XEvent* xev = event;
  XIDeviceEvent* xiev = static_cast<XIDeviceEvent*>(xev->xcookie.data);
  if (!factory->ShouldProcessXI2Event(xev))
    return;

  TRACE_EVENT1("input", "WindowTreeHostX11::DispatchXI2Event",
               "event_latency_us",
               (ui::EventTimeForNow() - ui::EventTimeFromNative(event)).
                 InMicroseconds());

  ui::EventType type = ui::EventTypeFromNative(xev);
  XEvent last_event;
  int num_coalesced = 0;

  switch (type) {
    case ui::ET_TOUCH_MOVED:
    case ui::ET_TOUCH_PRESSED:
    case ui::ET_TOUCH_CANCELLED:
    case ui::ET_TOUCH_RELEASED: {
      ui::TouchEvent touchev(xev);
      if (ui::DeviceDataManager::GetInstance()->TouchEventNeedsCalibrate(
              xiev->deviceid)) {
        touch_calibrate_->Calibrate(&touchev, bounds_);
      }
      TranslateAndDispatchLocatedEvent(&touchev);
      break;
    }
    case ui::ET_MOUSE_MOVED:
    case ui::ET_MOUSE_DRAGGED:
    case ui::ET_MOUSE_PRESSED:
    case ui::ET_MOUSE_RELEASED:
    case ui::ET_MOUSE_ENTERED:
    case ui::ET_MOUSE_EXITED: {
      if (type == ui::ET_MOUSE_MOVED || type == ui::ET_MOUSE_DRAGGED) {
        // If this is a motion event, we want to coalesce all pending motion
        // events that are at the top of the queue.
        num_coalesced = ui::CoalescePendingMotionEvents(xev, &last_event);
        if (num_coalesced > 0)
          xev = &last_event;
      }
      ui::MouseEvent mouseev(xev);
      TranslateAndDispatchLocatedEvent(&mouseev);
      break;
    }
    case ui::ET_MOUSEWHEEL: {
      ui::MouseWheelEvent mouseev(xev);
      TranslateAndDispatchLocatedEvent(&mouseev);
      break;
    }
    case ui::ET_SCROLL_FLING_START:
    case ui::ET_SCROLL_FLING_CANCEL:
    case ui::ET_SCROLL: {
      ui::ScrollEvent scrollev(xev);
      SendEventToProcessor(&scrollev);
      break;
    }
    case ui::ET_UMA_DATA:
      break;
    case ui::ET_UNKNOWN:
      break;
    default:
      NOTREACHED();
  }

  // If we coalesced an event we need to free its cookie.
  if (num_coalesced > 0)
    XFreeEventData(xev->xgeneric.display, &last_event.xcookie);
}

void WindowTreeHostX11::SetCursorInternal(gfx::NativeCursor cursor) {
  XDefineCursor(xdisplay_, xwindow_, cursor.platform());
}

void WindowTreeHostX11::OnConfigureNotify() {}

void WindowTreeHostX11::TranslateAndDispatchLocatedEvent(
    ui::LocatedEvent* event) {
  SendEventToProcessor(event);
}

// static
WindowTreeHost* WindowTreeHost::Create(const gfx::Rect& bounds) {
  return new WindowTreeHostX11(bounds);
}

// static
gfx::Size WindowTreeHost::GetNativeScreenSize() {
  ::XDisplay* xdisplay = gfx::GetXDisplay();
  return gfx::Size(DisplayWidth(xdisplay, 0), DisplayHeight(xdisplay, 0));
}

namespace test {

void SetUseOverrideRedirectWindowByDefault(bool override_redirect) {
  default_override_redirect = override_redirect;
}

}  // namespace test
}  // namespace aura
